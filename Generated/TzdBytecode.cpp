// ============================================================================
// TzdBytecode.cpp - Bytecode VM compiler and virtual machine
// Full implementation: AST -> Bytecode compiler (ANTLR visitor) + stack VM.
// ============================================================================

#include "TzdBytecode.h"
#include "TzdBytecodeJIT.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace {

// Local copy of the interpreter's string unescape (the original has internal
// linkage in TzdInterpreter.cpp and is not declared in any header).
std::string bcUnescapeString(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            switch (input[i + 1]) {
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case '\\': result += '\\'; break;
            case '"':  result += '"';  break;
            case '\'': result += '\''; break;
            case '0':  result += '\0'; break;
            default:   result += input[i + 1]; break;
            }
            ++i;
        } else {
            result += input[i];
        }
    }
    return result;
}

// Strip surrounding quotes from a string literal token's text.
std::string bcStripQuotes(const std::string& s) {
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'')) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Convert a TzdValue to its display string, reusing the interpreter's logic
// (which handles arrays/maps/instances/number formatting identically).
std::string bcValueToString(const TzdValue& v) {
    return TzdInterpreter::getAsString(std::any(v));
}

double bcAsDouble(const TzdValue& v) {
    return TzdInterpreter::getAsDoubleInternal(v);
}

bool bcIsIntLike(const TzdValue& v) {
    return v.type >= TzdValue::SBYTE && v.type <= TzdValue::LONG;
}

inline void releaseResourceIfNeeded(TzdValue& v) {
    if (v.instanceVal) {
        v.instanceVal->release();
        v.instanceVal = nullptr;
    }
    if (v.type == TzdValue::TENSOR && v.ptrVal) {
        tzdTensorRelease(v.ptrVal);
        v.ptrVal = nullptr;
    }
}

inline void copyValueFast(TzdValue& dst, const TzdValue& src) {
    releaseResourceIfNeeded(dst);
    dst.type = src.type;
    switch (src.type) {
    case TzdValue::INT: case TzdValue::LONG: case TzdValue::SHORT: case TzdValue::SBYTE:
    case TzdValue::UINT: case TzdValue::ULONG: case TzdValue::USHORT: case TzdValue::BYTE:
        dst.lVal = src.lVal;
        break;
    case TzdValue::DOUBLE: case TzdValue::FLOAT:
        dst.dVal = src.dVal;
        break;
    case TzdValue::BOOL:
        dst.bVal = src.bVal;
        break;
    case TzdValue::NONE:
        dst.lVal = 0;
        break;
    case TzdValue::STRING:
        dst.sVal = src.sVal;
        break;
    case TzdValue::POINTER:
        dst.ptrVal = src.ptrVal;
        break;
    case TzdValue::INSTANCE:
        dst.instanceVal = src.instanceVal;
        if (dst.instanceVal) dst.instanceVal->retain();
        break;
    default:
        dst = src;
        break;
    }
}

inline void moveValueFast(TzdValue& dst, TzdValue&& src) {
    releaseResourceIfNeeded(dst);
    dst.type = src.type;
    switch (src.type) {
    case TzdValue::INT: case TzdValue::LONG: case TzdValue::SHORT: case TzdValue::SBYTE:
    case TzdValue::UINT: case TzdValue::ULONG: case TzdValue::USHORT: case TzdValue::BYTE:
        dst.lVal = src.lVal;
        break;
    case TzdValue::DOUBLE: case TzdValue::FLOAT:
        dst.dVal = src.dVal;
        break;
    case TzdValue::BOOL:
        dst.bVal = src.bVal;
        break;
    case TzdValue::NONE:
        dst.lVal = 0;
        break;
    case TzdValue::STRING:
        dst.sVal = std::move(src.sVal);
        break;
    case TzdValue::POINTER:
        dst.ptrVal = src.ptrVal;
        break;
    case TzdValue::INSTANCE:
        dst.instanceVal = src.instanceVal;
        src.instanceVal = nullptr;
        break;
    default:
        dst = std::move(src);
        break;
    }
    src.type = TzdValue::NONE;
    src.ptrVal = nullptr;
}

// Unwrap ParenExprContext to inner expression
static TzdLangParser::ExpressionContext* unwrapExpr(TzdLangParser::ExpressionContext* expr) {
    while (expr) {
        if (auto p = dynamic_cast<TzdLangParser::ParenExprContext*>(expr)) {
            expr = p->expression();
        } else {
            break;
        }
    }
    return expr;
}

// Try to extract an integer constant from an expression (e.g. 1, -1, 100)
static bool tryGetIntLiteral(TzdLangParser::ExpressionContext* expr, int64_t& outVal) {
    expr = unwrapExpr(expr);
    if (!expr) return false;
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(expr)) {
        if (auto intCtx = dynamic_cast<TzdLangParser::IntExprContext*>(atomExpr->atom())) {
            std::string raw = intCtx->getText();
            try {
                outVal = std::stoll(raw, nullptr, 0);
                return true;
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

// Check if an expression is an identifier referring to the specified variable name
static bool isSameVarId(TzdLangParser::ExpressionContext* expr, const std::string& varName) {
    expr = unwrapExpr(expr);
    if (!expr) return false;
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(expr)) {
        if (auto idCtx = dynamic_cast<TzdLangParser::IdExprContext*>(atomExpr->atom())) {
            return idCtx->IDENTIFIER()->getText() == varName;
        }
    }
    return false;
}

// Check if either operand requires BIGINT/RATIONAL arithmetic
static bool bcNeedsBigint(const TzdValue& a, const TzdValue& b) {
    return a.type == TzdValue::BIGINT || b.type == TzdValue::BIGINT ||
           a.type == TzdValue::RATIONAL || b.type == TzdValue::RATIONAL;
}

// Perform BIGINT/RATIONAL binary arithmetic: op is '+','-','*','/','%'
static TzdValue bcBigintArith(const TzdValue& a, const TzdValue& b, char op) {
    bool useRational = needs_rational(a, b);
    std::string r;
    if (useRational) {
        std::string sa = to_rational_str(a), sb = to_rational_str(b);
        switch (op) {
            case '+': r = rational_add(sa, sb); break;
            case '-': r = rational_sub(sa, sb); break;
            case '*': r = rational_mul(sa, sb); break;
            case '/': {
                std::string bn, bd; rational_parse(sb, bn, bd);
                if (bn == "0") return TzdValue(0LL);
                r = rational_div(sa, sb); break;
            }
            default: r = bigint_mod(to_bigint_str(a), to_bigint_str(b)); break;
        }
    } else {
        std::string sa = to_bigint_str(a), sb = to_bigint_str(b);
        switch (op) {
            case '+': r = bigint_add(sa, sb); break;
            case '-': r = bigint_sub(sa, sb); break;
            case '*': r = bigint_mul(sa, sb); break;
            case '/': if (sb == "0" || sb == "-0") return TzdValue(0LL);
                      r = bigint_div(sa, sb); break;
            case '%': if (sb == "0" || sb == "-0") return TzdValue(0LL);
                      r = bigint_mod(sa, sb); break;
        }
    }
    if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
    TzdValue v;
    v.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
    v.sVal = std::move(r);
    return v;
}

} // namespace

// ============================================================================
// TzdBytecodeCompiler
// ============================================================================
TzdBytecodeCompiler::TzdBytecodeCompiler() : m_localIndex(0) {}

int TzdBytecodeCompiler::addConstant(double v) {
    ConstEntry e;
    e.type = ConstEntry::DOUBLE;
    e.dVal = v;
    m_module.constants.push_back(e);
    return (int)m_module.constants.size() - 1;
}

int TzdBytecodeCompiler::addConstant(const std::string& v) {
    for (size_t i = 0; i < m_module.constants.size(); ++i) {
        if (m_module.constants[i].type == ConstEntry::STRING &&
            m_module.constants[i].sVal == v) {
            return (int)i;
        }
    }
    ConstEntry e;
    e.type = ConstEntry::STRING;
    e.sVal = v;
    m_module.constants.push_back(e);
    return (int)m_module.constants.size() - 1;
}

int TzdBytecodeCompiler::addConstant(int64_t v) {
    ConstEntry e;
    e.type = ConstEntry::INT;
    e.iVal = v;
    m_module.constants.push_back(e);
    return (int)m_module.constants.size() - 1;
}

int TzdBytecodeCompiler::addConstant(bool v) {
    ConstEntry e;
    e.type = ConstEntry::BOOL;
    e.bVal = v;
    m_module.constants.push_back(e);
    return (int)m_module.constants.size() - 1;
}

int TzdBytecodeCompiler::addConstant(const ConstEntry& e) {
    m_module.constants.push_back(e);
    return (int)m_module.constants.size() - 1;
}

void TzdBytecodeCompiler::emit(OpCode op, int32_t arg1, int32_t arg2) {
    if (!m_currentFunc) return;
    auto& code = m_currentFunc->code;
    // Peephole optimization for POP
    if (op == OpCode::POP && code.size() >= 2) {
        // Pattern 1: DUP, STORE_LOCAL -> STORE_LOCAL
        if (code.back().op == OpCode::STORE_LOCAL && code[code.size() - 2].op == OpCode::DUP) {
            Instruction store = code.back();
            code.pop_back();
            code.pop_back();
            code.push_back(store);
            return;
        }
        // Pattern 2: DUP, STORE_VAR -> STORE_VAR
        if (code.back().op == OpCode::STORE_VAR && code[code.size() - 2].op == OpCode::DUP) {
            Instruction store = code.back();
            code.pop_back();
            code.pop_back();
            code.push_back(store);
            return;
        }
        // Pattern 3: INC_LOCAL, LOAD_LOCAL -> INC_LOCAL
        if (code.back().op == OpCode::LOAD_LOCAL && code[code.size() - 2].op == OpCode::INC_LOCAL &&
            code.back().arg1 == code[code.size() - 2].arg1) {
            code.pop_back();
            return;
        }
        // Pattern 4: LOAD_LOCAL, INC_LOCAL -> INC_LOCAL
        if (code.back().op == OpCode::INC_LOCAL && code[code.size() - 2].op == OpCode::LOAD_LOCAL &&
            code.back().arg1 == code[code.size() - 2].arg1) {
            Instruction inc = code.back();
            code.pop_back();
            code.pop_back();
            code.push_back(inc);
            return;
        }
    }
    Instruction instr;
    instr.op = op;
    instr.arg1 = arg1;
    instr.arg2 = arg2;
    code.push_back(instr);
}

size_t TzdBytecodeCompiler::here() const {
    return m_currentFunc ? m_currentFunc->code.size() : 0;
}

size_t TzdBytecodeCompiler::emitJump(OpCode op) {
    size_t idx = here();
    emit(op, 0);
    return idx;
}

void TzdBytecodeCompiler::patchJump(size_t instrIdx, int32_t target) {
    if (!m_currentFunc || instrIdx >= m_currentFunc->code.size()) return;
    m_currentFunc->code[instrIdx].arg1 = target;
}

int TzdBytecodeCompiler::resolveLocal(const std::string& name) const {
    for (auto it = m_scopeStack.rbegin(); it != m_scopeStack.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return -1;
}

int TzdBytecodeCompiler::declareLocal(const std::string& name) {
    int slot = m_localIndex++;
    if (m_scopeStack.empty()) m_scopeStack.push_back({});
    m_scopeStack.back()[name] = slot;
    return slot;
}

void TzdBytecodeCompiler::setLocalType(int slot, LocalType type) {
    if (!m_currentFunc) return;
    // Grow localTypes if needed
    if ((int)m_currentFunc->localTypes.size() <= slot)
        m_currentFunc->localTypes.resize(slot + 1, LocalType::Unknown);
    m_currentFunc->localTypes[slot] = type;
}

void TzdBytecodeCompiler::pushScope() {
    m_scopeStack.push_back({});
}

void TzdBytecodeCompiler::popScope() {
    if (!m_scopeStack.empty()) m_scopeStack.pop_back();
}

void TzdBytecodeCompiler::beginFunction(const std::string& name, int paramCount) {
    m_module.functions.push_back(BytecodeFunc{});
    m_currentFuncIdx = m_module.functions.size() - 1;
    m_currentFunc = &m_module.functions.back();
    m_currentFunc->name = name;
    m_currentFunc->paramCount = paramCount;
    m_currentFunc->localCount = 0;
    m_localIndex = 0;
    m_scopeStack.clear();
    m_scopeStack.push_back({}); // function scope
    m_moduleFuncNames[name] = m_currentFuncIdx;
    m_module.funcIndex[name] = m_currentFuncIdx;
}

void TzdBytecodeCompiler::endFunction(bool /*emitReturn*/) {
    if (m_currentFunc) {
        // Guarantee every function ends with an explicit return.
        if (m_currentFunc->code.empty() ||
            m_currentFunc->code.back().op != OpCode::RET_VOID) {
            emit(OpCode::RET_VOID);
        }
        m_currentFunc->localCount = m_localIndex;
    }
    m_currentFunc = nullptr;
    m_currentFuncIdx = (size_t)-1;
    m_scopeStack.clear();
}

bool TzdBytecodeCompiler::isModuleFunction(const std::string& name) const {
    return m_moduleFuncNames.find(name) != m_moduleFuncNames.end();
}

std::string TzdBytecodeCompiler::getQualifiedName(
    TzdLangParser::QualifiedNameContext* ctx) const {
    return ctx ? ctx->getText() : "";
}

std::vector<std::string> TzdBytecodeCompiler::extractParamNames(
    TzdLangParser::ParamListContext* pl) const {
    std::vector<std::string> names;
    if (!pl) return names;
    for (auto p : pl->param()) {
        if (p->IDENTIFIER()) names.push_back(p->IDENTIFIER()->getText());
        else names.push_back(p->getText());
    }
    return names;
}

BytecodeModule TzdBytecodeCompiler::compile(TzdLangParser::ProgramContext* tree, const std::string& sourceCode) {
    m_module = BytecodeModule{};
    m_module.name = "main";
    m_module.sourceCode = sourceCode;

    // Pre-register all top-level function names so forward references resolve.
    if (tree) {
        for (auto stmt : tree->statement()) {
            if (auto funStmt = dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt)) {
                auto fdecl = funStmt->functionDeclaration();
                if (!fdecl) continue;
                std::string fname = fdecl->IDENTIFIER()->getText();
                m_moduleFuncNames[fname] = (size_t)-1; // placeholder
            }
        }
    }

    // Phase 1: emit top-level code into __main__.
    beginFunction("__main__", 0);
    if (tree) visit(tree);
    endFunction(true);

    // Phase 2: compile each function body into its own BytecodeFunc.
    if (tree) {
        for (auto stmt : tree->statement()) {
            if (auto funStmt = dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt)) {
                visit(funStmt);
            }
        }
    }

    // Phase 3: verify bytecode and compute maxStackDepth
    std::string verifyErr;
    if (!verifyModule(m_module, &verifyErr)) {
        std::cerr << "[Bytecode Verifier] " << verifyErr << std::endl;
    }

    return m_module;
}

// ============================================================================
// Bytecode Verifier — validates jump targets, constant indices, stack underflow,
// and computes maxStackDepth for each function.
// ============================================================================
static int stackDelta(OpCode op, int32_t arg1, int32_t arg2) {
    switch (op) {
    // Constants: +1
    case OpCode::PUSH_DOUBLE: case OpCode::PUSH_INT:
    case OpCode::PUSH_STRING: case OpCode::PUSH_BOOL:
    case OpCode::PUSH_NULL:
        return 1;

    // Variables: LOAD +1, STORE -1
    case OpCode::LOAD_LOCAL: case OpCode::LOAD_VAR:
        return 1;
    case OpCode::STORE_LOCAL: case OpCode::STORE_VAR:
        return -1;

    // Binary arithmetic/comparison: -1 (pop 2, push 1)
    case OpCode::ADD: case OpCode::SUB: case OpCode::MUL: case OpCode::DIV:
    case OpCode::MOD: case OpCode::POW:
    case OpCode::EQ: case OpCode::NE: case OpCode::LT: case OpCode::LE:
    case OpCode::GT: case OpCode::GE:
    case OpCode::AND: case OpCode::OR:
        return -1;

    // Unary: 0 (pop 1, push 1)
    case OpCode::NEG: case OpCode::NOT:
        return 0;

    // Control flow: JMP_FALSE/JMP_TRUE pop 1
    case OpCode::JMP:
        return 0;
    case OpCode::JMP_FALSE: case OpCode::JMP_TRUE:
        return -1;

    // Calls: pop argc, pop callee/obj, push result = -argc (CALL_FUNC) or -argc-1+1 = -argc (CALL_METHOD)
    case OpCode::CALL_FUNC:
        return -arg2 + 1;  // pop argc args, push result
    case OpCode::CALL_NATIVE:
        return -arg2 + 1;
    case OpCode::CALL_METHOD:
        return -arg2 - 1 + 1;  // pop argc args + receiver, push result
    case OpCode::CALL_VALUE:
        return -arg1 - 1 + 1;  // pop argc args + callee, push result

    // RET: pop 1, RET_VOID: 0
    case OpCode::RET:
        return -1;
    case OpCode::RET_VOID:
        return 0;

    // Stack ops: POP -1, DUP +1
    case OpCode::POP:
        return -1;
    case OpCode::DUP:
        return 1;

    // PRINT: pop argc, push null = -argc + 1
    case OpCode::PRINT:
        return -arg1 + 1;

    // THROW: pop 1
    case OpCode::THROW:
        return -1;

    // SCOPE: no effect
    case OpCode::SCOPE_PUSH: case OpCode::SCOPE_POP:
        return 0;

    // Member: LOAD pops 1 pushes 1 = 0; STORE pops 2 pushes 1 = -1
    case OpCode::LOAD_MEMBER:
        return 0;
    case OpCode::STORE_MEMBER:
        return -1;

    // Index: LOAD pops 2 pushes 1 = -1; STORE pops 3 pushes 2 = -1
    case OpCode::LOAD_INDEX:
        return -1;
    case OpCode::STORE_INDEX:
        return -1;  // pops val, idx, container; pushes val + container

    // NEW_OBJECT: pop argc, push instance
    case OpCode::NEW_OBJECT:
        return -arg2 + 1;

    // MAKE_ARRAY: pop count, push array
    case OpCode::MAKE_ARRAY:
        return -arg1 + 1;

    // TRY/TRY_END: no stack effect
    case OpCode::TRY: case OpCode::TRY_END:
        return 0;

    case OpCode::INC_LOCAL:
        return 0;

    case OpCode::HALT:
        return 0;
    }
    return 0;
}

static bool isJumpOp(OpCode op) {
    return op == OpCode::JMP || op == OpCode::JMP_FALSE || op == OpCode::JMP_TRUE;
}

static bool usesConstantOp(OpCode op) {
    switch (op) {
    case OpCode::PUSH_DOUBLE: case OpCode::PUSH_INT:
    case OpCode::PUSH_STRING: case OpCode::PUSH_BOOL:
    case OpCode::LOAD_VAR: case OpCode::STORE_VAR:
    case OpCode::CALL_FUNC: case OpCode::CALL_NATIVE: case OpCode::CALL_METHOD:
    case OpCode::LOAD_MEMBER: case OpCode::STORE_MEMBER:
    case OpCode::NEW_OBJECT:
        return true;
    default:
        return false;
    }
}

static bool usesLocalOp(OpCode op) {
    return op == OpCode::LOAD_LOCAL || op == OpCode::STORE_LOCAL || op == OpCode::INC_LOCAL;
}

bool TzdBytecodeCompiler::verifyModule(BytecodeModule& mod, std::string* errMsg) {
    for (size_t fi = 0; fi < mod.functions.size(); ++fi) {
        BytecodeFunc& f = mod.functions[fi];
        int32_t stackDepth = 0;
        int32_t maxDepth = 0;
        int32_t localLimit = (f.localCount > f.paramCount ? f.localCount : f.paramCount);

        for (size_t i = 0; i < f.code.size(); ++i) {
            const Instruction& ins = f.code[i];

            // 1. Constant index bounds check
            if (usesConstantOp(ins.op) && ins.arg1 >= (int32_t)mod.constants.size()) {
                if (errMsg) *errMsg = "Bytecode verification failed: constant index " +
                    std::to_string(ins.arg1) + " out of bounds in function " + f.name;
                return false;
            }

            // 2. Jump target bounds check
            if (isJumpOp(ins.op) && (ins.arg1 < 0 || ins.arg1 >= (int32_t)f.code.size())) {
                if (errMsg) *errMsg = "Bytecode verification failed: jump target " +
                    std::to_string(ins.arg1) + " out of bounds in function " + f.name;
                return false;
            }

            // 3. Local slot index bounds check
            if (usesLocalOp(ins.op) && ins.arg1 >= localLimit) {
                if (errMsg) *errMsg = "Bytecode verification failed: local slot " +
                    std::to_string(ins.arg1) + " out of bounds in function " + f.name;
                return false;
            }

            // 4. Stack underflow check
            int delta = stackDelta(ins.op, ins.arg1, ins.arg2);
            stackDepth += delta;
            if (stackDepth < 0) {
                if (errMsg) *errMsg = "Bytecode verification failed: stack underflow at " +
                    f.name + ":" + std::to_string(i);
                return false;
            }
            if (stackDepth > maxDepth) maxDepth = stackDepth;
        }

        // 5. Compute and store maxStackDepth
        f.maxStackDepth = maxDepth;

        // 6. Initialize localTypes with Unknown
        if (f.localTypes.empty() && localLimit > 0) {
            f.localTypes.resize(localLimit, LocalType::Unknown);
        }
    }
    return true;
}
std::any TzdBytecodeCompiler::visitProgram(TzdLangParser::ProgramContext* ctx) {
    for (auto stmt : ctx->statement()) {
        // Skip declaration statements — they're handled by the interpreter
        // during executeBytecodeFile (via sourceCode re-parsing).
        if (dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt)) continue;
        if (dynamic_cast<TzdLangParser::ClassDeclStmtContext*>(stmt)) continue;
        if (dynamic_cast<TzdLangParser::EnumDeclStmtContext*>(stmt)) continue;
        if (dynamic_cast<TzdLangParser::AnnotationDeclStmtContext*>(stmt)) continue;
        if (dynamic_cast<TzdLangParser::NativeFunDeclStmtContext*>(stmt)) continue;
        if (dynamic_cast<TzdLangParser::ImportStmtContext*>(stmt)) continue;
        visit(stmt);
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitBlockStmt(TzdLangParser::BlockStmtContext* ctx) {
    if (ctx->block()) visit(ctx->block());
    return std::any();
}

std::any TzdBytecodeCompiler::visitBlock(TzdLangParser::BlockContext* ctx) {
    pushScope();
    for (auto stmt : ctx->statement()) {
        visit(stmt);
    }
    popScope();
    return std::any();
}

std::any TzdBytecodeCompiler::visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) {
    auto fdecl = ctx->functionDeclaration();
    if (!fdecl) return std::any();

    std::string fname = fdecl->IDENTIFIER()->getText();
    auto params = extractParamNames(fdecl->paramList());

    // Save the caller's compilation context (use index, not pointer,
    // because beginFunction may push_back and invalidate the pointer).
    size_t savedFuncIdx = m_currentFuncIdx;
    int savedLocal = m_localIndex;
    auto savedScopes = m_scopeStack;
    auto savedLoops = m_loops;

    beginFunction(fname, (int)params.size());
    for (auto& p : params) declareLocal(p);
    m_currentFunc->paramNames = params; // for closure scope storage

    if (fdecl->block()) visit(fdecl->block());

    endFunction(true);

    // Restore caller context (re-fetch pointer from stable index).
    m_currentFuncIdx = savedFuncIdx;
    m_currentFunc = (savedFuncIdx != (size_t)-1) ? &m_module.functions[savedFuncIdx] : nullptr;
    m_localIndex = savedLocal;
    m_scopeStack = std::move(savedScopes);
    m_loops = std::move(savedLoops);
    return std::any();
}

std::any TzdBytecodeCompiler::visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) {
    return visit(ctx->variableDeclaration());
}

std::any TzdBytecodeCompiler::visitVariableDeclaration(
    TzdLangParser::VariableDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();

    // In __main__ function, use global storage so all functions can access
    if (m_currentFunc && m_currentFunc->name == "__main__") {
        if (ctx->expression()) {
            visit(ctx->expression());
            emit(OpCode::STORE_VAR, addConstant(name));
        }
        return std::any();
    }

    int slot = declareLocal(name);

    // Type inference: infer local variable type from declared type annotation
    if (ctx->typeType()) {
        std::string typeName = ctx->typeType()->getText();
        if (typeName == "int" || typeName == "long" || typeName == "i32" || typeName == "i64")
            setLocalType(slot, LocalType::Int64);
        else if (typeName == "float" || typeName == "double" || typeName == "f32" || typeName == "f64")
            setLocalType(slot, LocalType::Float64);
        else
            setLocalType(slot, LocalType::ObjRef);
    } else {
        // Implicit `var` — will be inferred at runtime; mark as Unknown
        setLocalType(slot, LocalType::Unknown);
    }

    if (ctx->expression()) {
        visit(ctx->expression());          // push init value
        emit(OpCode::DUP);                 // duplicate: one for local, one for scope
        emit(OpCode::STORE_LOCAL, slot);   // fast local access
        emit(OpCode::STORE_VAR, addConstant(name)); // for nested function access
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitForInit(TzdLangParser::ForInitContext* ctx) {
    if (ctx->variableDeclaration()) return visit(ctx->variableDeclaration());
    if (ctx->expression()) return visit(ctx->expression());
    return std::any();
}

std::any TzdBytecodeCompiler::visitExprStmt(TzdLangParser::ExprStmtContext* ctx) {
    visit(ctx->expression());
    emit(OpCode::POP); // expression statements discard their result
    return std::any();
}

std::any TzdBytecodeCompiler::visitEmptyStmt(TzdLangParser::EmptyStmtContext* /*ctx*/) {
    return std::any();
}

std::any TzdBytecodeCompiler::visitIfStmt(TzdLangParser::IfStmtContext* ctx) {
    visit(ctx->expression());                    // condition
    size_t jmpToElse = emitJump(OpCode::JMP_FALSE);

    visit(ctx->statement(0));                   // then-branch

    size_t jmpToEnd = emitJump(OpCode::JMP);
    patchJump(jmpToElse, (int32_t)here());       // else-branch start

    if (ctx->KW_ELSE()) {
        visit(ctx->statement(1));
    }
    patchJump(jmpToEnd, (int32_t)here());
    return std::any();
}

std::any TzdBytecodeCompiler::visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) {
    size_t condIp = here();
    visit(ctx->expression());                    // condition
    size_t exitJmp = emitJump(OpCode::JMP_FALSE);

    LoopContext lc;
    lc.continueTarget = (int)condIp;             // continue -> re-test condition
    m_loops.push_back(std::move(lc));

    visit(ctx->statement());                     // body

    emit(OpCode::JMP, (int32_t)condIp);          // loop back
    size_t endIp = here();

    for (size_t j : m_loops.back().breakJumps) patchJump(j, (int32_t)endIp);
    for (size_t j : m_loops.back().continueJumps) patchJump(j, (int32_t)condIp);
    m_loops.pop_back();
    patchJump(exitJmp, (int32_t)endIp);
    return std::any();
}

std::any TzdBytecodeCompiler::visitForStmt(TzdLangParser::ForStmtContext* ctx) {
    pushScope();
    if (ctx->forInit()) visit(ctx->forInit());

    size_t condIp = here();
    size_t exitJmp = (size_t)-1;
    if (ctx->cond) {
        visit(ctx->cond);
        exitJmp = emitJump(OpCode::JMP_FALSE);
    }

    LoopContext lc;
    lc.continueTarget = -1;                      // resolved after the body
    m_loops.push_back(std::move(lc));

    if (ctx->statement()) visit(ctx->statement());

    size_t stepIp = here();                       // continue jumps here
    m_loops.back().continueTarget = (int)stepIp;
    if (ctx->step) visit(ctx->step);
    emit(OpCode::JMP, (int32_t)condIp);

    size_t endIp = here();
    for (size_t j : m_loops.back().breakJumps) patchJump(j, (int32_t)endIp);
    for (size_t j : m_loops.back().continueJumps) patchJump(j, (int32_t)stepIp);
    m_loops.pop_back();
    if (exitJmp != (size_t)-1) patchJump(exitJmp, (int32_t)endIp);

    popScope();
    return std::any();
}

std::any TzdBytecodeCompiler::visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) {
    if (ctx->expression()) {
        visit(ctx->expression());
        emit(OpCode::RET);
    } else {
        emit(OpCode::RET_VOID);
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitBreakStmt(TzdLangParser::BreakStmtContext* /*ctx*/) {
    if (m_loops.empty()) return std::any();
    size_t j = emitJump(OpCode::JMP);
    m_loops.back().breakJumps.push_back(j);
    return std::any();
}

std::any TzdBytecodeCompiler::visitContinueStmt(TzdLangParser::ContinueStmtContext* /*ctx*/) {
    if (m_loops.empty()) return std::any();
    size_t j = emitJump(OpCode::JMP);
    m_loops.back().continueJumps.push_back(j);
    return std::any();
}

std::any TzdBytecodeCompiler::visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) {
    // Compile switch as if/else chain
    visit(ctx->expression());           // push switch value
    std::vector<size_t> endJumps;
    for (auto sc : ctx->switchCase()) {
        // if (value == caseValue) { body }
        emit(OpCode::DUP);              // duplicate switch value
        visit(sc->expression());         // push case value
        emit(OpCode::EQ);               // compare
        size_t skipBody = emitJump(OpCode::JMP_FALSE);
        emit(OpCode::POP);              // remove duplicated switch value
        for (auto stmt : sc->statement()) visit(stmt);
        endJumps.push_back(emitJump(OpCode::JMP));
        patchJump(skipBody, (int32_t)here());
    }
    // default case
    if (ctx->switchDefault()) {
        emit(OpCode::POP);              // remove switch value
        for (auto stmt : ctx->switchDefault()->statement()) visit(stmt);
    } else {
        emit(OpCode::POP);              // remove switch value
    }
    for (size_t j : endJumps) patchJump(j, (int32_t)here());
    return std::any();
}

std::any TzdBytecodeCompiler::visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) {
    // Compile lambda as anonymous function
    static int lambdaCounter = 0;
    std::string lambdaName = "__lambda_" + std::to_string(lambdaCounter++);

    auto params = extractParamNames(ctx->paramList());

    size_t savedFuncIdx = m_currentFuncIdx;
    int savedLocal = m_localIndex;
    auto savedScopes = m_scopeStack;
    auto savedLoops = m_loops;

    beginFunction(lambdaName, (int)params.size());
    for (auto& p : params) declareLocal(p);
    m_currentFunc->paramNames = params;

    if (ctx->block()) visit(ctx->block());

    endFunction(true);

    m_currentFuncIdx = savedFuncIdx;
    m_currentFunc = (savedFuncIdx != (size_t)-1) ? &m_module.functions[savedFuncIdx] : nullptr;
    m_localIndex = savedLocal;
    m_scopeStack = std::move(savedScopes);
    m_loops = std::move(savedLoops);

    // Push function value onto stack
    emit(OpCode::LOAD_VAR, addConstant(lambdaName));
    return std::any();
}

std::any TzdBytecodeCompiler::visitSuperExpr(TzdLangParser::SuperExprContext* ctx) {
    // super(args) -> call parent constructor via interpreter
    int argCount = ctx->exprList() ? (int)ctx->exprList()->expression().size() : 0;
    if (ctx->exprList())
        for (auto e : ctx->exprList()->expression()) visit(e);
    // Emit CALL_FUNC "super" which falls through to interpreter
    emit(OpCode::CALL_FUNC, addConstant("super"), argCount);
    return std::any();
}

std::any TzdBytecodeCompiler::visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) {
    // x in Type -> call native "isType"
    visit(ctx->expression());  // push value
    // Push type name as string
    std::string typeName = ctx->qualifiedName() ? ctx->qualifiedName()->getText()
                                                  : ctx->typeType()->getText();
    emit(OpCode::PUSH_STRING, addConstant(typeName));
    emit(OpCode::CALL_FUNC, addConstant("isType"), 2);
    return std::any();
}

std::any TzdBytecodeCompiler::visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) {
    visit(ctx->expression());
    emit(OpCode::THROW);
    return std::any();
}

std::any TzdBytecodeCompiler::visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) {
    // Layout:
    //   TRY <catchIp> <catchSlot>     (forward-ref, patched below)
    //   <try-block>
    //   TRY_END
    //   JMP <end>
    // catch:
    //   STORE_LOCAL <catchSlot>        (binds thrown value)
    //   <catch-block>
    // end:
    size_t tryInstr = emitJump(OpCode::TRY);

    if (ctx->block(0)) visit(ctx->block(0));  // try block

    emit(OpCode::TRY_END);
    size_t jmpPastCatch = emitJump(OpCode::JMP);

    size_t catchIp = here();
    pushScope();
    std::string catchVar = ctx->IDENTIFIER() ? ctx->IDENTIFIER()->getText() : "__ex";
    int catchSlot = declareLocal(catchVar);
    emit(OpCode::STORE_LOCAL, catchSlot);      // bind thrown value at entry

    if (ctx->block(1)) visit(ctx->block(1));  // catch block
    popScope();

    size_t endIp = here();
    patchJump(tryInstr, (int32_t)catchIp);
    if (m_currentFunc && tryInstr < m_currentFunc->code.size()) {
        m_currentFunc->code[tryInstr].arg2 = catchSlot;
    }
    patchJump(jmpPastCatch, (int32_t)endIp);
    return std::any();
}

// ============================================================================
// Expression visitors
// ============================================================================
std::any TzdBytecodeCompiler::visitIntExpr(TzdLangParser::IntExprContext* ctx) {
    std::string raw = ctx->getText();
    // Fast path: skip stoll for >19 digit numbers (stoll reads the WHOLE string before throwing)
    size_t digitStart = (raw.size() > 0 && (raw[0] == '-' || raw[0] == '+')) ? 1 : 0;
    bool isHex = (raw.size() > 2 + digitStart && raw[digitStart] == '0' &&
                  (raw[digitStart+1] == 'x' || raw[digitStart+1] == 'X'));
    if (!isHex && raw.size() - digitStart > 19) {
        // Directly store as BIGINT constant — no stoll, no validation loop
        ConstEntry ce;
        ce.type = ConstEntry::BIGINT;
        ce.sVal = std::move(raw);
        ce.iVal = 0;
        ce.dVal = 0;
        ce.bVal = false;
        emit(OpCode::PUSH_INT, addConstant(ce));
        return std::any();
    }
    int64_t val = 0;
    try {
        val = std::stoll(raw, nullptr, 0);
        emit(OpCode::PUSH_INT, addConstant(val));
    } catch (...) {
        // Overflow: check if it's a valid big integer (all digits, optional leading -)
        bool valid = true;
        size_t start = (raw[0] == '-') ? 1 : 0;
        for (size_t i = start; i < raw.size(); ++i) {
            if (raw[i] < '0' || raw[i] > '9') { valid = false; break; }
        }
        if (valid && raw.size() > start + 1) {
            ConstEntry ce;
            ce.type = ConstEntry::BIGINT;
            ce.sVal = raw;
            ce.iVal = 0;
            ce.dVal = 0;
            ce.bVal = false;
            emit(OpCode::PUSH_INT, addConstant(ce));
        } else {
            try { val = (int64_t)std::stod(raw); } catch (...) { val = 0; }
            emit(OpCode::PUSH_INT, addConstant(val));
        }
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitFloatExpr(TzdLangParser::FloatExprContext* ctx) {
    double v = 0.0;
    try { v = std::stod(ctx->getText()); } catch (...) {}
    emit(OpCode::PUSH_DOUBLE, addConstant(v));
    return std::any();
}

std::any TzdBytecodeCompiler::visitStringExpr(TzdLangParser::StringExprContext* ctx) {
    std::string s = bcUnescapeString(bcStripQuotes(ctx->getText()));
    emit(OpCode::PUSH_STRING, addConstant(s));
    return std::any();
}

std::any TzdBytecodeCompiler::visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* /*ctx*/) {
    emit(OpCode::PUSH_BOOL, addConstant(true));
    return std::any();
}

std::any TzdBytecodeCompiler::visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* /*ctx*/) {
    emit(OpCode::PUSH_BOOL, addConstant(false));
    return std::any();
}

std::any TzdBytecodeCompiler::visitNullExpr(TzdLangParser::NullExprContext* /*ctx*/) {
    emit(OpCode::PUSH_NULL);
    return std::any();
}

std::any TzdBytecodeCompiler::visitIdExpr(TzdLangParser::IdExprContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    int slot = resolveLocal(name);
    if (slot >= 0) {
        emit(OpCode::LOAD_LOCAL, slot);
    } else {
        emit(OpCode::LOAD_VAR, addConstant(name));
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitParenExpr(TzdLangParser::ParenExprContext* ctx) {
    return visit(ctx->expression());
}

std::any TzdBytecodeCompiler::visitAtomExpr(TzdLangParser::AtomExprContext* ctx) {
    return visit(ctx->atom());
}

std::any TzdBytecodeCompiler::visitCastExpr(TzdLangParser::CastExprContext* ctx) {
    // Casts are dynamically typed at the bytecode level; just evaluate the
    // inner expression. (Type coercion is handled by the runtime values.)
    return visit(ctx->expression());
}

std::any TzdBytecodeCompiler::visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) {
    visit(ctx->expression(0));
    visit(ctx->expression(1));
    emit(ctx->PLUS() ? OpCode::ADD : OpCode::SUB);
    return std::any();
}

std::any TzdBytecodeCompiler::visitMultiplicativeExpr(
    TzdLangParser::MultiplicativeExprContext* ctx) {
    visit(ctx->expression(0));
    visit(ctx->expression(1));
    if (ctx->MUL()) emit(OpCode::MUL);
    else if (ctx->DIV()) emit(OpCode::DIV);
    else emit(OpCode::MOD);
    return std::any();
}

std::any TzdBytecodeCompiler::visitPowerExpr(TzdLangParser::PowerExprContext* ctx) {
    visit(ctx->expression(0));
    visit(ctx->expression(1));
    emit(OpCode::POW);
    return std::any();
}

std::any TzdBytecodeCompiler::visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) {
    visit(ctx->expression());
    if (ctx->MINUS()) emit(OpCode::NEG);
    else if (ctx->NOT()) emit(OpCode::NOT);
    else if (ctx->GXXX()) {
        // gxxx = square root; approximate via POW of 0.5.
        emit(OpCode::PUSH_DOUBLE, addConstant(0.5));
        emit(OpCode::POW);
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitRelationalExpr(
    TzdLangParser::RelationalExprContext* ctx) {
    visit(ctx->expression(0));
    visit(ctx->expression(1));
    if (ctx->GT()) emit(OpCode::GT);
    else if (ctx->LT()) emit(OpCode::LT);
    else if (ctx->GE()) emit(OpCode::GE);
    else emit(OpCode::LE);
    return std::any();
}

std::any TzdBytecodeCompiler::visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) {
    visit(ctx->expression(0));
    visit(ctx->expression(1));
    emit(ctx->EEQ() ? OpCode::EQ : OpCode::NE);
    return std::any();
}

std::any TzdBytecodeCompiler::visitLogicalAndExpr(
    TzdLangParser::LogicalAndExprContext* ctx) {
    visit(ctx->expression(0));
    size_t jf1 = emitJump(OpCode::JMP_FALSE); // short-circuit if left is false
    visit(ctx->expression(1));
    size_t jf2 = emitJump(OpCode::JMP_FALSE);
    emit(OpCode::PUSH_BOOL, addConstant(true));
    size_t skip = emitJump(OpCode::JMP);
    size_t falseLabel = here();
    patchJump(jf1, (int32_t)falseLabel);
    patchJump(jf2, (int32_t)falseLabel);
    emit(OpCode::PUSH_BOOL, addConstant(false));
    patchJump(skip, (int32_t)here());
    return std::any();
}

std::any TzdBytecodeCompiler::visitLogicalOrExpr(
    TzdLangParser::LogicalOrExprContext* ctx) {
    visit(ctx->expression(0));
    size_t jt1 = emitJump(OpCode::JMP_TRUE); // short-circuit if left is true
    visit(ctx->expression(1));
    size_t jt2 = emitJump(OpCode::JMP_TRUE);
    emit(OpCode::PUSH_BOOL, addConstant(false));
    size_t skip = emitJump(OpCode::JMP);
    size_t trueLabel = here();
    patchJump(jt1, (int32_t)trueLabel);
    patchJump(jt2, (int32_t)trueLabel);
    emit(OpCode::PUSH_BOOL, addConstant(true));
    patchJump(skip, (int32_t)here());
    return std::any();
}

std::any TzdBytecodeCompiler::visitAssignmentExpr(
    TzdLangParser::AssignmentExprContext* ctx) {
    auto lhs = ctx->expression(0);
    bool isCompound = !ctx->ASSIGN();
    OpCode compoundOp = OpCode::ADD;
    if (ctx->PLUS_ASSIGN()) compoundOp = OpCode::ADD;
    else if (ctx->MIN_ASSIGN()) compoundOp = OpCode::SUB;
    else if (ctx->MUL_ASSIGN()) compoundOp = OpCode::MUL;
    else if (ctx->DIV_ASSIGN()) compoundOp = OpCode::DIV;

    // --- Index assignment: arr[i] = v ---
    if (auto idx = dynamic_cast<TzdLangParser::IndexExprContext*>(lhs)) {
        // Extract container variable name (may be wrapped in AtomExpr)
        std::string containerName;
        auto containerExpr = idx->expression(0);
        // Unwrap AtomExpr to get the inner atom
        if (auto atomWrapper = dynamic_cast<TzdLangParser::AtomExprContext*>(containerExpr)) {
            auto innerAtom = atomWrapper->atom();
            if (auto containerId = dynamic_cast<TzdLangParser::IdExprContext*>(innerAtom)) {
                containerName = containerId->IDENTIFIER()->getText();
            }
        }
        int containerSlot = !containerName.empty() ? resolveLocal(containerName) : -1;

        if (isCompound) {
            visit(idx->expression(0));
            visit(idx->expression(1));
            emit(OpCode::LOAD_INDEX);
            int t = declareLocal("__t_assign");
            emit(OpCode::STORE_LOCAL, t);
            visit(idx->expression(0));
            visit(idx->expression(1));
            emit(OpCode::LOAD_LOCAL, t);
            visit(ctx->expression(1));
            emit(compoundOp);
            emit(OpCode::STORE_INDEX);
        } else {
            visit(idx->expression(0));
            visit(idx->expression(1));
            visit(ctx->expression(1));
            emit(OpCode::STORE_INDEX);
        }
        // After STORE_INDEX, stack has: [value, modified_container]
        // Store modified container back to variable
        if (containerSlot >= 0) {
            emit(OpCode::STORE_LOCAL, containerSlot);
        } else if (!containerName.empty()) {
            emit(OpCode::STORE_VAR, addConstant(containerName));
        } else {
            emit(OpCode::POP);
        }
        return std::any();
    }

    // --- Member / identifier assignment via AtomExpr ---
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhs)) {
        auto atom = atomExpr->atom();
        if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atom)) {
            int nameIdx = addConstant(mem->IDENTIFIER()->getText());
            visit(mem->atom());                   // object
            emit(OpCode::DUP);                    // keep object for store
            if (isCompound) {
                emit(OpCode::LOAD_MEMBER, nameIdx);   // old
                visit(ctx->expression(1));            // rhs
                emit(compoundOp);                     // new
            } else {
                visit(ctx->expression(1));            // value
            }
            emit(OpCode::DUP);                    // result
            emit(OpCode::STORE_MEMBER, nameIdx);
            return std::any();
        }
        if (auto id = dynamic_cast<TzdLangParser::IdExprContext*>(atom)) {
            std::string name = id->IDENTIFIER()->getText();
            int slot = resolveLocal(name);

            // Fast path: i += delta or i -= delta on local slot
            int64_t delta = 0;
            if (slot >= 0 && isCompound && (ctx->PLUS_ASSIGN() || ctx->MIN_ASSIGN())) {
                if (tryGetIntLiteral(ctx->expression(1), delta)) {
                    int32_t d = ctx->PLUS_ASSIGN() ? (int32_t)delta : -(int32_t)delta;
                    emit(OpCode::INC_LOCAL, slot, d);
                    emit(OpCode::LOAD_LOCAL, slot);
                    return std::any();
                }
            }

            // Fast path: i = i + delta or i = i - delta (or i = delta + i) on local slot
            if (slot >= 0 && !isCompound) {
                if (auto addCtx = dynamic_cast<TzdLangParser::AdditiveExprContext*>(ctx->expression(1))) {
                    if (addCtx->PLUS()) {
                        if (isSameVarId(addCtx->expression(0), name) && tryGetIntLiteral(addCtx->expression(1), delta)) {
                            emit(OpCode::INC_LOCAL, slot, (int32_t)delta);
                            emit(OpCode::LOAD_LOCAL, slot);
                            return std::any();
                        } else if (isSameVarId(addCtx->expression(1), name) && tryGetIntLiteral(addCtx->expression(0), delta)) {
                            emit(OpCode::INC_LOCAL, slot, (int32_t)delta);
                            emit(OpCode::LOAD_LOCAL, slot);
                            return std::any();
                        }
                    } else if (addCtx->MINUS()) {
                        if (isSameVarId(addCtx->expression(0), name) && tryGetIntLiteral(addCtx->expression(1), delta)) {
                            emit(OpCode::INC_LOCAL, slot, -(int32_t)delta);
                            emit(OpCode::LOAD_LOCAL, slot);
                            return std::any();
                        }
                    }
                }
            }

            if (isCompound) {
                if (slot >= 0) emit(OpCode::LOAD_LOCAL, slot);
                else emit(OpCode::LOAD_VAR, addConstant(name));
                visit(ctx->expression(1));
                emit(compoundOp);
            } else {
                visit(ctx->expression(1));
            }
            emit(OpCode::DUP);
            if (slot >= 0) emit(OpCode::STORE_LOCAL, slot);
            else emit(OpCode::STORE_VAR, addConstant(name));
            return std::any();
        }
    }

    // Fallback: evaluate RHS and leave on stack (best effort).
    visit(ctx->expression(1));
    return std::any();
}

std::any TzdBytecodeCompiler::visitCallExpr(TzdLangParser::CallExprContext* ctx) {
    auto atom = ctx->atom();
    int argCount = ctx->exprList() ? (int)ctx->exprList()->expression().size() : 0;

    // obj.method(args) -> method call
    if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atom)) {
        visit(mem->atom());                       // receiver
        if (ctx->exprList())
            for (auto e : ctx->exprList()->expression()) visit(e);
        emit(OpCode::CALL_METHOD, addConstant(mem->IDENTIFIER()->getText()), argCount);
        return std::any();
    }

    // plain name(args) -> function call by name
    if (auto id = dynamic_cast<TzdLangParser::IdExprContext*>(atom)) {
        if (ctx->exprList())
            for (auto e : ctx->exprList()->expression()) visit(e);
        emit(OpCode::CALL_FUNC, addConstant(id->IDENTIFIER()->getText()), argCount);
        return std::any();
    }

    // (callee-expr)(args) -> call value on stack
    visit(atom);                                 // callee value
    if (ctx->exprList())
        for (auto e : ctx->exprList()->expression()) visit(e);
    emit(OpCode::CALL_VALUE, argCount);
    return std::any();
}

std::any TzdBytecodeCompiler::visitMemberAccessExpr(
    TzdLangParser::MemberAccessExprContext* ctx) {
    visit(ctx->atom());
    emit(OpCode::LOAD_MEMBER, addConstant(ctx->IDENTIFIER()->getText()));
    return std::any();
}

std::any TzdBytecodeCompiler::visitIndexExpr(TzdLangParser::IndexExprContext* ctx) {
    visit(ctx->expression(0));                    // container
    visit(ctx->expression(1));                    // index
    emit(OpCode::LOAD_INDEX);
    return std::any();
}

std::any TzdBytecodeCompiler::visitArrayLiteralExpr(
    TzdLangParser::ArrayLiteralExprContext* ctx) {
    int count = 0;
    if (ctx->exprList()) {
        count = (int)ctx->exprList()->expression().size();
        for (auto e : ctx->exprList()->expression()) visit(e);
    }
    emit(OpCode::MAKE_ARRAY, count);
    return std::any();
}

std::any TzdBytecodeCompiler::visitMapLiteralExpr(
    TzdLangParser::MapLiteralExprContext* ctx) {
    emit(OpCode::PUSH_NULL);
    return std::any();
}

std::any TzdBytecodeCompiler::visitNewExpr(TzdLangParser::NewExprContext* ctx) {
    std::string className = ctx->qualifiedName()->getText();
    int argCount = ctx->exprList() ? (int)ctx->exprList()->expression().size() : 0;
    if (ctx->exprList())
        for (auto e : ctx->exprList()->expression()) visit(e);
    emit(OpCode::NEW_OBJECT, addConstant(className), argCount);
    return std::any();
}

std::any TzdBytecodeCompiler::visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) {
    int count = 0;
    auto pf = ctx->printFunction();
    if (pf && pf->exprList()) {
        count = (int)pf->exprList()->expression().size();
        for (auto e : pf->exprList()->expression()) visit(e);
    }
    emit(OpCode::PRINT, count);
    return std::any();
}

std::any TzdBytecodeCompiler::visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) {
    bool isInc = ctx->INC() != nullptr;
    auto lhs = ctx->expression();

    if (auto idx = dynamic_cast<TzdLangParser::IndexExprContext*>(lhs)) {
        // ++arr[i] -> load old, +/-1, store, result new
        visit(idx->expression(0));
        visit(idx->expression(1));
        emit(OpCode::LOAD_INDEX);
        emit(OpCode::PUSH_INT, addConstant((int64_t)1));
        emit(isInc ? OpCode::ADD : OpCode::SUB);
        int t = declareLocal("__t_pre");
        emit(OpCode::STORE_LOCAL, t);
        visit(idx->expression(0));
        visit(idx->expression(1));
        emit(OpCode::LOAD_LOCAL, t);
        emit(OpCode::STORE_INDEX);
        emit(OpCode::LOAD_LOCAL, t);
        return std::any();
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhs)) {
        auto atom = atomExpr->atom();
        if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atom)) {
            int nameIdx = addConstant(mem->IDENTIFIER()->getText());
            visit(mem->atom());
            emit(OpCode::DUP);
            emit(OpCode::LOAD_MEMBER, nameIdx);
            emit(OpCode::PUSH_INT, addConstant((int64_t)1));
            emit(isInc ? OpCode::ADD : OpCode::SUB);
            emit(OpCode::DUP);
            emit(OpCode::STORE_MEMBER, nameIdx);
            return std::any();
        }
        if (auto id = dynamic_cast<TzdLangParser::IdExprContext*>(atom)) {
            std::string name = id->IDENTIFIER()->getText();
            int slot = resolveLocal(name);
            if (slot >= 0) {
                emit(OpCode::INC_LOCAL, slot, isInc ? 1 : -1);
                emit(OpCode::LOAD_LOCAL, slot);
                return std::any();
            }
            emit(OpCode::LOAD_VAR, addConstant(name));
            emit(OpCode::PUSH_INT, addConstant((int64_t)1));
            emit(isInc ? OpCode::ADD : OpCode::SUB);
            emit(OpCode::DUP);
            emit(OpCode::STORE_VAR, addConstant(name));
            return std::any();
        }
    }
    return std::any();
}

std::any TzdBytecodeCompiler::visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) {
    bool isInc = ctx->INC() != nullptr;
    auto lhs = ctx->expression();

    if (auto idx = dynamic_cast<TzdLangParser::IndexExprContext*>(lhs)) {
        // arr[i]++ -> result old; arr[i] = old+/-1
        visit(idx->expression(0));
        visit(idx->expression(1));
        emit(OpCode::LOAD_INDEX);
        int t = declareLocal("__t_post");
        emit(OpCode::STORE_LOCAL, t);            // save old
        visit(idx->expression(0));
        visit(idx->expression(1));
        emit(OpCode::LOAD_LOCAL, t);
        emit(OpCode::PUSH_INT, addConstant((int64_t)1));
        emit(isInc ? OpCode::ADD : OpCode::SUB);
        emit(OpCode::STORE_INDEX);
        emit(OpCode::LOAD_LOCAL, t);             // result = old
        return std::any();
    }
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhs)) {
        auto atom = atomExpr->atom();
        if (auto mem = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atom)) {
            int nameIdx = addConstant(mem->IDENTIFIER()->getText());
            visit(mem->atom());
            emit(OpCode::DUP);
            emit(OpCode::LOAD_MEMBER, nameIdx);  // old
            emit(OpCode::DUP);                    // keep old as result
            emit(OpCode::PUSH_INT, addConstant((int64_t)1));
            emit(isInc ? OpCode::ADD : OpCode::SUB);
            emit(OpCode::STORE_MEMBER, nameIdx);
            return std::any();
        }
        if (auto id = dynamic_cast<TzdLangParser::IdExprContext*>(atom)) {
            std::string name = id->IDENTIFIER()->getText();
            int slot = resolveLocal(name);
            if (slot >= 0) {
                emit(OpCode::LOAD_LOCAL, slot);
                emit(OpCode::INC_LOCAL, slot, isInc ? 1 : -1);
                return std::any();
            }
            emit(OpCode::LOAD_VAR, addConstant(name));
            emit(OpCode::DUP);                   // keep old as result
            emit(OpCode::PUSH_INT, addConstant((int64_t)1));
            emit(isInc ? OpCode::ADD : OpCode::SUB);
            emit(OpCode::STORE_VAR, addConstant(name));
            return std::any();
        }
    }
    return std::any();
}

// ----------------------------------------------------------------------------
// .tzdc file serialization (preserved from the original stub).
// ----------------------------------------------------------------------------
bool TzdBytecodeCompiler::saveToFile(const BytecodeModule& module, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;

    uint32_t magic = BytecodeModule::MAGIC;
    uint32_t version = BytecodeModule::VERSION;
    file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Save source code for class/enum/annotation declarations
    uint32_t srcLen = (uint32_t)module.sourceCode.size();
    file.write(reinterpret_cast<const char*>(&srcLen), sizeof(srcLen));
    if (srcLen > 0) file.write(module.sourceCode.data(), srcLen);

    uint32_t constCount = (uint32_t)module.constants.size();
    file.write(reinterpret_cast<const char*>(&constCount), sizeof(constCount));
    for (const auto& c : module.constants) {
        uint8_t type = (uint8_t)c.type;
        file.write(reinterpret_cast<const char*>(&type), sizeof(type));
        if (c.type == ConstEntry::DOUBLE) {
            file.write(reinterpret_cast<const char*>(&c.dVal), sizeof(c.dVal));
        } else if (c.type == ConstEntry::STRING) {
            uint32_t len = (uint32_t)c.sVal.size();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(c.sVal.data(), len);
        } else if (c.type == ConstEntry::INT) {
            file.write(reinterpret_cast<const char*>(&c.iVal), sizeof(c.iVal));
        } else if (c.type == ConstEntry::BOOL) {
            uint8_t b = c.bVal ? 1 : 0;
            file.write(reinterpret_cast<const char*>(&b), sizeof(b));
        }
    }

    uint32_t funcCount = (uint32_t)module.functions.size();
    file.write(reinterpret_cast<const char*>(&funcCount), sizeof(funcCount));
    for (const auto& f : module.functions) {
        uint32_t nameLen = (uint32_t)f.name.size();
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(f.name.data(), nameLen);
        file.write(reinterpret_cast<const char*>(&f.paramCount), sizeof(f.paramCount));
        file.write(reinterpret_cast<const char*>(&f.localCount), sizeof(f.localCount));
        file.write(reinterpret_cast<const char*>(&f.maxStackDepth), sizeof(f.maxStackDepth));
        // Save param names for closure scope support
        uint32_t pnCount = (uint32_t)f.paramNames.size();
        file.write(reinterpret_cast<const char*>(&pnCount), sizeof(pnCount));
        for (const auto& pn : f.paramNames) {
            uint32_t pnl = (uint32_t)pn.size();
            file.write(reinterpret_cast<const char*>(&pnl), sizeof(pnl));
            file.write(pn.data(), pnl);
        }
        uint32_t codeLen = (uint32_t)f.code.size();
        file.write(reinterpret_cast<const char*>(&codeLen), sizeof(codeLen));
        for (const auto& instr : f.code) {
            file.write(reinterpret_cast<const char*>(&instr.op), sizeof(instr.op));
            file.write(reinterpret_cast<const char*>(&instr.arg1), sizeof(instr.arg1));
            file.write(reinterpret_cast<const char*>(&instr.arg2), sizeof(instr.arg2));
        }
    }
    return true;
}

BytecodeModule TzdBytecodeCompiler::loadFromFile(const std::string& path) {
    BytecodeModule module;
    std::ifstream file(path, std::ios::binary);
    if (!file) return module;

    uint32_t magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (magic != BytecodeModule::MAGIC) return module;

    // Load source code (for class/enum/annotation declarations)
    if (version >= 2) {
        uint32_t srcLen;
        file.read(reinterpret_cast<char*>(&srcLen), sizeof(srcLen));
        if (srcLen > 0) {
            module.sourceCode.resize(srcLen);
            file.read(&module.sourceCode[0], srcLen);
        }
    }

    uint32_t constCount;
    file.read(reinterpret_cast<char*>(&constCount), sizeof(constCount));
    for (uint32_t i = 0; i < constCount; i++) {
        ConstEntry c;
        uint8_t type;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        c.type = (ConstEntry::Type)type;
        if (c.type == ConstEntry::DOUBLE) {
            file.read(reinterpret_cast<char*>(&c.dVal), sizeof(c.dVal));
        } else if (c.type == ConstEntry::STRING) {
            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            c.sVal.resize(len);
            file.read(&c.sVal[0], len);
        } else if (c.type == ConstEntry::INT) {
            file.read(reinterpret_cast<char*>(&c.iVal), sizeof(c.iVal));
        } else if (c.type == ConstEntry::BOOL) {
            uint8_t b;
            file.read(reinterpret_cast<char*>(&b), sizeof(b));
            c.bVal = b != 0;
        }
        module.constants.push_back(c);
    }

    uint32_t funcCount;
    file.read(reinterpret_cast<char*>(&funcCount), sizeof(funcCount));
    for (uint32_t i = 0; i < funcCount; i++) {
        BytecodeFunc f;
        uint32_t nameLen;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        f.name.resize(nameLen);
        file.read(&f.name[0], nameLen);
        file.read(reinterpret_cast<char*>(&f.paramCount), sizeof(f.paramCount));
        file.read(reinterpret_cast<char*>(&f.localCount), sizeof(f.localCount));
        file.read(reinterpret_cast<char*>(&f.maxStackDepth), sizeof(f.maxStackDepth));
        // Load param names for closure scope support
        uint32_t pnCount;
        file.read(reinterpret_cast<char*>(&pnCount), sizeof(pnCount));
        for (uint32_t p = 0; p < pnCount; p++) {
            uint32_t pnl;
            file.read(reinterpret_cast<char*>(&pnl), sizeof(pnl));
            std::string pn;
            pn.resize(pnl);
            file.read(&pn[0], pnl);
            f.paramNames.push_back(pn);
        }
        uint32_t codeLen;
        file.read(reinterpret_cast<char*>(&codeLen), sizeof(codeLen));
        f.code.resize(codeLen);
        for (uint32_t j = 0; j < codeLen; j++) {
            file.read(reinterpret_cast<char*>(&f.code[j].op), sizeof(f.code[j].op));
            file.read(reinterpret_cast<char*>(&f.code[j].arg1), sizeof(f.code[j].arg1));
            file.read(reinterpret_cast<char*>(&f.code[j].arg2), sizeof(f.code[j].arg2));
        }
        module.funcIndex[f.name] = i;
        module.functions.push_back(f);
    }

    return module;
}

// ============================================================================
// TzdBytecodeVM
// ============================================================================
TzdBytecodeVM::TzdBytecodeVM(TzdInterpreter* interp) : m_interp(interp) {
    m_stack.resize(65536);
    m_locals.resize(16384);
    m_sp = 0;
    m_localTop = 0;
}

TzdValue TzdBytecodeVM::execute(const BytecodeModule& module) {
    if (module.functions.empty()) return TzdValue();
    m_module = &module;
    for (size_t i = 0; i < m_sp; ++i) {
        if (m_stack[i].instanceVal) {
            m_stack[i].instanceVal->release();
            m_stack[i].instanceVal = nullptr;
        }
        m_stack[i].type = TzdValue::NONE;
    }
    m_sp = 0;
    for (size_t i = 0; i < m_localTop; ++i) {
        if (m_locals[i].instanceVal) {
            m_locals[i].instanceVal->release();
            m_locals[i].instanceVal = nullptr;
        }
        m_locals[i].type = TzdValue::NONE;
    }
    m_localTop = 0;
    m_handlers.clear();
    try {
        return runBytecodeFunc(module, 0, {});
    } catch (const TzdThrowException& e) {
        std::cerr << "Exception in thread \"main\" Tzd: " << bcValueToString(e.value)
                  << std::endl;
        return TzdValue();
    } catch (const std::exception& e) {
        std::cerr << "Exception in thread \"main\" Tzd: " << e.what() << std::endl;
        return TzdValue();
    }
}

TzdValue TzdBytecodeVM::callFunction(const BytecodeModule& module,
                                     const std::string& funcName,
                                     const std::vector<TzdValue>& args) {
    m_module = &module;
    auto it = module.funcIndex.find(funcName);
    if (it == module.funcIndex.end()) {
        if (m_interp) {
            try {
                TzdValue fv = m_interp->getVariable(funcName, nullptr);
                return m_interp->callFunction(fv, args);
            } catch (...) {
                return TzdValue();
            }
        }
        return TzdValue();
    }
    try {
        return runBytecodeFunc(module, it->second, args);
    } catch (const TzdThrowException& e) {
        std::cerr << "Exception in thread \"main\" Tzd: " << bcValueToString(e.value)
                  << std::endl;
        return TzdValue();
    } catch (const std::exception& e) {
        std::cerr << "Exception in thread \"main\" Tzd: " << e.what() << std::endl;
        return TzdValue();
    }
}

TzdValue TzdBytecodeVM::callValue(const BytecodeModule& module, const TzdValue& callee,
                                  const std::vector<TzdValue>& args) {
    // Bytecode function referenced by name (funcBody is null sentinel).
    if (callee.type == TzdValue::FUNCTION && callee.funcBody == nullptr &&
        !callee.name.empty()) {
        auto it = module.funcIndex.find(callee.name);
        if (it != module.funcIndex.end()) {
            return runBytecodeFunc(module, it->second, args);
        }
    }
    if (callee.type == TzdValue::FUNCTION || callee.type == TzdValue::NATIVE_FUNCTION) {
        if (m_interp) return m_interp->callFunction(callee, args);
    }
    throw std::runtime_error("Attempt to call a non-function value");
}

TzdValue TzdBytecodeVM::getMember(const TzdValue& obj, const std::string& name) const {
    TzdSelector sel = tzdInternSelector(name);
    if (obj.type == TzdValue::INSTANCE && obj.instanceVal) {
        return obj.instanceVal->getMember(sel, name);
    }
    if (obj.type == TzdValue::CLASS_DEF && obj.classDefVal) {
        TzdClassDef* cls = obj.classDefVal;
        if (const TzdMemberSlot* slot = cls->tzdDispatch.find(sel)) {
            if (slot->staticValue) return *slot->staticValue;
            if (slot->method) {
                if (slot->method->templateVal) {
                    TzdValue fv = *(slot->method->templateVal);
                    fv.jittedPtr = slot->method->jittedPtr;
                    return fv;
                }
                TzdValue fv(slot->method->name, slot->method->params, slot->method->body);
                fv.paramTypes = slot->method->paramTypes;
                fv.jittedPtr = slot->method->jittedPtr;
                return fv;
            }
        }
        if (cls->staticValues.count(name)) return cls->staticValues.at(name);
        ClassMethod* m = cls->findMethod(name);
        if (m) {
            TzdValue fv(m->name, m->params, m->body);
            fv.paramTypes = m->paramTypes;
            fv.jittedPtr = m->jittedPtr;
            return fv;
        }
        throw std::runtime_error("Class '" + cls->fullName + "' has no member: " + name);
    }
    if (obj.type == TzdValue::MAP) {
        auto it = obj.mapVal.find(name);
        if (it != obj.mapVal.end()) return it->second;
        return TzdValue();
    }
    throw std::runtime_error("Cannot access member '" + name + "' on value of type " +
                             bcValueToString(obj));
}

void TzdBytecodeVM::setMember(const TzdValue& obj, const std::string& name,
                              const TzdValue& val) const {
    TzdSelector sel = tzdInternSelector(name);
    if (obj.type == TzdValue::INSTANCE && obj.instanceVal) {
        obj.instanceVal->setMember(sel, name, val);
        return;
    }
    if (obj.type == TzdValue::CLASS_DEF && obj.classDefVal) {
        if (const TzdMemberSlot* slot = obj.classDefVal->tzdDispatch.find(sel)) {
            if (slot->staticValue) {
                *slot->staticValue = val;
                return;
            }
        }
        obj.classDefVal->staticValues[name] = val;
        return;
    }
    if (obj.type == TzdValue::MAP) {
        const_cast<TzdValue&>(obj).mapVal[name] = val;
        return;
    }
    throw std::runtime_error("Cannot set member '" + name + "' on value of type " +
                             bcValueToString(obj));
}

static const char* opName(OpCode op) {
    switch (op) {
    case OpCode::PUSH_DOUBLE: return "PUSH_D";
    case OpCode::PUSH_INT: return "PUSH_I";
    case OpCode::PUSH_STRING: return "PUSH_S";
    case OpCode::PUSH_BOOL: return "PUSH_B";
    case OpCode::PUSH_NULL: return "PUSH_N";
    case OpCode::LOAD_LOCAL: return "LOAD_L";
    case OpCode::STORE_LOCAL: return "STORE_L";
    case OpCode::LOAD_VAR: return "LOAD_V";
    case OpCode::STORE_VAR: return "STORE_V";
    case OpCode::ADD: return "ADD";
    case OpCode::SUB: return "SUB";
    case OpCode::MUL: return "MUL";
    case OpCode::DIV: return "DIV";
    case OpCode::MOD: return "MOD";
    case OpCode::POW: return "POW";
    case OpCode::NEG: return "NEG";
    case OpCode::EQ: return "EQ";
    case OpCode::NE: return "NE";
    case OpCode::LT: return "LT";
    case OpCode::LE: return "LE";
    case OpCode::GT: return "GT";
    case OpCode::GE: return "GE";
    case OpCode::AND: return "AND";
    case OpCode::OR: return "OR";
    case OpCode::NOT: return "NOT";
    case OpCode::JMP: return "JMP";
    case OpCode::JMP_FALSE: return "JMP_F";
    case OpCode::JMP_TRUE: return "JMP_T";
    case OpCode::CALL_FUNC: return "CALL_F";
    case OpCode::CALL_NATIVE: return "CALL_N";
    case OpCode::CALL_VALUE: return "CALL_V";
    case OpCode::CALL_METHOD: return "CALL_M";
    case OpCode::RET: return "RET";
    case OpCode::RET_VOID: return "RET_V";
    case OpCode::POP: return "POP";
    case OpCode::DUP: return "DUP";
    case OpCode::PRINT: return "PRINT";
    case OpCode::THROW: return "THROW";
    case OpCode::SCOPE_PUSH: return "SCP_P";
    case OpCode::SCOPE_POP: return "SCP_P";
    case OpCode::LOAD_MEMBER: return "LD_M";
    case OpCode::STORE_MEMBER: return "ST_M";
    case OpCode::LOAD_INDEX: return "LD_I";
    case OpCode::STORE_INDEX: return "ST_I";
    case OpCode::NEW_OBJECT: return "NEW";
    case OpCode::MAKE_ARRAY: return "MK_ARR";
    case OpCode::TRY: return "TRY";
    case OpCode::TRY_END: return "TRY_END";
    case OpCode::INC_LOCAL: return "INC_L";
    case OpCode::HALT: return "HALT";
    }
    return "?";
}

void TzdBytecodeVM::disassemble(const BytecodeModule& module) const {
    for (size_t fi = 0; fi < module.functions.size(); ++fi) {
        const BytecodeFunc& f = module.functions[fi];
        std::cerr << "=== func[" << fi << "] " << f.name
                  << " params=" << f.paramCount << " locals=" << f.localCount
                  << " code=" << f.code.size() << " ===\n";
        for (size_t i = 0; i < f.code.size(); ++i) {
            const Instruction& ins = f.code[i];
            std::cerr << "  " << i << ": " << opName(ins.op)
                      << " " << ins.arg1 << " " << ins.arg2 << "\n";
        }
    }
}

TzdValue TzdBytecodeVM::runBytecodeFunc(const BytecodeModule& module,
                                        size_t funcIndex,
                                        const std::vector<TzdValue>& args) {
    return runBytecodeFunc(module, funcIndex, args.data(), args.size());
}

struct VMCallFrame {
    size_t funcIndex;
    const Instruction* code;
    size_t codeSize;
    size_t ip;
    size_t localBase;
    size_t stackBase;
    size_t handlerBase;
};

TzdValue TzdBytecodeVM::runBytecodeFunc(const BytecodeModule& module,
                                        size_t funcIndex,
                                        const TzdValue* argsData,
                                        size_t argCount) {
    if (funcIndex >= module.functions.size()) return TzdValue();
    const BytecodeFunc* curFunc = &module.functions[funcIndex];
    const Instruction* code = curFunc->code.data();
    size_t codeSize = curFunc->code.size();

    // Call stack for iterative dispatch (eliminates recursive C++ runBytecodeFunc)
    std::vector<VMCallFrame> callFrames;
    callFrames.reserve(128);

    size_t initialLocalBase = m_localTop;
    size_t initialStackBase = m_sp;
    size_t initialHandlerBase = m_handlers.size();

    size_t currentFuncIdx = funcIndex;
    size_t localBase = m_localTop;
    int localsNeeded = (curFunc->localCount > curFunc->paramCount) ? curFunc->localCount : curFunc->paramCount;
    size_t needed = localBase + (size_t)localsNeeded + 1;
    if (m_locals.size() < needed) {
        m_locals.resize(needed * 2);
    }
    m_localTop = needed;

    // Copy initial arguments into locals
    for (int i = 0; i < curFunc->paramCount && i < (int)argCount; ++i) {
        copyValueFast(m_locals[localBase + i], argsData[i]);
    }
    for (size_t i = localBase + argCount; i < needed; ++i) {
        releaseResourceIfNeeded(m_locals[i]);
        m_locals[i].type = TzdValue::NONE;
        m_locals[i].lVal = 0;
    }

    size_t stackBase = m_sp;
    size_t handlerBase = m_handlers.size();
    size_t ip = 0;
    TzdValue result; // default null

    auto pushInt = [&](int64_t v) {
        if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
        auto& slot = m_stack[m_sp++];
        releaseResourceIfNeeded(slot);
        slot.type = TzdValue::INT;
        slot.lVal = v;
    };
    auto pushDouble = [&](double v) {
        if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
        auto& slot = m_stack[m_sp++];
        releaseResourceIfNeeded(slot);
        slot.type = TzdValue::DOUBLE;
        slot.dVal = v;
    };
    auto pushBool = [&](bool v) {
        if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
        auto& slot = m_stack[m_sp++];
        releaseResourceIfNeeded(slot);
        slot.type = TzdValue::BOOL;
        slot.bVal = v;
    };
    auto pushNull = [&]() {
        if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
        auto& slot = m_stack[m_sp++];
        releaseResourceIfNeeded(slot);
        slot.type = TzdValue::NONE;
        slot.lVal = 0;
    };
    auto pushValue = [&](const TzdValue& v) {
        if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
        copyValueFast(m_stack[m_sp++], v);
    };
    auto pushValueMove = [&](TzdValue&& v) {
        if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
        moveValueFast(m_stack[m_sp++], std::move(v));
    };
    auto pop = [&](TzdValue& out) -> bool {
        if (m_sp <= stackBase) return false;
        moveValueFast(out, std::move(m_stack[--m_sp]));
        return true;
    };

    for (;;) {
    try {
    while (ip < codeSize) {
        const Instruction& instr = code[ip];
        switch (instr.op) {
            // ---- Constants ----
            case OpCode::PUSH_DOUBLE: {
                const ConstEntry& c = module.constants[instr.arg1];
                pushDouble(c.dVal);
                ++ip; break;
            }
            case OpCode::PUSH_INT: {
                const ConstEntry& c = module.constants[instr.arg1];
                if (c.type == ConstEntry::BIGINT) {
                    TzdValue v;
                    v.type = TzdValue::BIGINT;
                    v.sVal = c.sVal;
                    pushValueMove(std::move(v));
                } else {
                    pushInt(c.iVal);
                }
                ++ip; break;
            }
            case OpCode::PUSH_STRING: {
                const ConstEntry& c = module.constants[instr.arg1];
                if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
                auto& slot = m_stack[m_sp++];
                releaseResourceIfNeeded(slot);
                slot.type = TzdValue::STRING;
                slot.sVal = c.sVal;
                ++ip; break;
            }
            case OpCode::PUSH_BOOL: {
                const ConstEntry& c = module.constants[instr.arg1];
                pushBool(c.bVal);
                ++ip; break;
            }
            case OpCode::PUSH_NULL: {
                pushNull();
                ++ip; break;
            }

            // ---- Variables ----
            case OpCode::LOAD_LOCAL: {
                const TzdValue& v = m_locals[localBase + instr.arg1];
                switch (v.type) {
                case TzdValue::INT: case TzdValue::LONG: case TzdValue::SHORT:
                case TzdValue::SBYTE:
                    pushInt(v.lVal); break;
                case TzdValue::DOUBLE: case TzdValue::FLOAT:
                    pushDouble(v.dVal); break;
                case TzdValue::BOOL:
                    pushBool(v.bVal); break;
                case TzdValue::NONE:
                    pushNull(); break;
                case TzdValue::STRING: {
                    if (m_sp >= m_stack.size()) m_stack.resize(m_stack.size() * 2);
                    auto& slot = m_stack[m_sp++];
                    releaseResourceIfNeeded(slot);
                    slot.type = TzdValue::STRING;
                    slot.sVal = v.sVal;
                    break;
                }
                default:
                    pushValue(v); break;
                }
                ++ip; break;
            }
            case OpCode::STORE_LOCAL: {
                if (m_sp > stackBase) {
                    moveValueFast(m_locals[localBase + instr.arg1], std::move(m_stack[--m_sp]));
                }
                ++ip; break;
            }
            case OpCode::INC_LOCAL: {
                TzdValue& v = m_locals[localBase + instr.arg1];
                if (v.type == TzdValue::INT || v.type == TzdValue::LONG ||
                    v.type == TzdValue::SHORT || v.type == TzdValue::SBYTE) {
                    v.lVal += instr.arg2;
                } else if (v.type == TzdValue::DOUBLE || v.type == TzdValue::FLOAT) {
                    v.dVal += instr.arg2;
                }
                ++ip; break;
            }
            case OpCode::LOAD_VAR: {
                const std::string& name = module.constants[instr.arg1].sVal;
                auto it = module.funcIndex.find(name);
                if (it != module.funcIndex.end()) {
                    TzdValue fv;
                    fv.type = TzdValue::FUNCTION;
                    fv.name = name;
                    pushValueMove(std::move(fv));
                } else {
                    pushValue(m_interp->getVariable(name, nullptr));
                }
                ++ip; break;
            }
            case OpCode::STORE_VAR: {
                const std::string& name = module.constants[instr.arg1].sVal;
                if (m_sp > stackBase) {
                    m_interp->setVariable(name, m_stack[--m_sp]);
                }
                ++ip; break;
            }

            // ---- Arithmetic ----
            case OpCode::ADD: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        a.lVal += b.lVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        a.dVal += b.dVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        a.type = TzdValue::DOUBLE;
                        a.dVal = (double)a.lVal + b.dVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        a.dVal += (double)b.lVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::STRING && b.type == TzdValue::STRING) {
                        a.sVal += b.sVal;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (a.type == TzdValue::STRING || b.type == TzdValue::STRING) {
                    pushValueMove(TzdValue(bcValueToString(a) + bcValueToString(b)));
                } else if (bcIsIntLike(a) && bcIsIntLike(b)) {
                    pushInt(a.lVal + b.lVal);
                } else if (bcNeedsBigint(a, b)) {
                    pushValueMove(bcBigintArith(a, b, '+'));
                } else {
                    pushDouble(bcAsDouble(a) + bcAsDouble(b));
                }
                ++ip; break;
            }
            case OpCode::SUB: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        a.lVal -= b.lVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        a.dVal -= b.dVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        a.type = TzdValue::DOUBLE;
                        a.dVal = (double)a.lVal - b.dVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        a.dVal -= (double)b.lVal;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b))
                    pushInt(a.lVal - b.lVal);
                else if (bcNeedsBigint(a, b))
                    pushValueMove(bcBigintArith(a, b, '-'));
                else
                    pushDouble(bcAsDouble(a) - bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::MUL: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        a.lVal *= b.lVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        a.dVal *= b.dVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        a.type = TzdValue::DOUBLE;
                        a.dVal = (double)a.lVal * b.dVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        a.dVal *= (double)b.lVal;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b))
                    pushInt(a.lVal * b.lVal);
                else if (bcNeedsBigint(a, b))
                    pushValueMove(bcBigintArith(a, b, '*'));
                else
                    pushDouble(bcAsDouble(a) * bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::DIV: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT && b.lVal != 0) {
                        a.lVal /= b.lVal;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE && b.dVal != 0.0) {
                        a.dVal /= b.dVal;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b)) {
                    if (b.lVal == 0) throw std::runtime_error("Division by zero");
                    pushInt(a.lVal / b.lVal);
                } else if (bcNeedsBigint(a, b)) {
                    pushValueMove(bcBigintArith(a, b, '/'));
                } else {
                    double r = bcAsDouble(b);
                    if (r == 0.0) throw std::runtime_error("Division by zero");
                    pushDouble(bcAsDouble(a) / r);
                }
                ++ip; break;
            }
            case OpCode::MOD: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT && b.lVal != 0) {
                        a.lVal %= b.lVal;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b) && b.lVal != 0) {
                    pushInt(a.lVal % b.lVal);
                } else if (bcNeedsBigint(a, b)) {
                    pushValueMove(bcBigintArith(a, b, '%'));
                } else {
                    double r = bcAsDouble(b);
                    if (r == 0.0) throw std::runtime_error("Modulo by zero");
                    pushDouble(std::fmod(bcAsDouble(a), r));
                }
                ++ip; break;
            }
            case OpCode::POW: {
                TzdValue b, a; pop(b); pop(a);
                pushDouble(std::pow(bcAsDouble(a), bcAsDouble(b)));
                ++ip; break;
            }
            case OpCode::NEG: {
                if (m_sp > stackBase) {
                    TzdValue& a = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT) {
                        a.lVal = -a.lVal;
                        ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE) {
                        a.dVal = -a.dVal;
                        ++ip; break;
                    }
                }
                TzdValue a; pop(a);
                if (bcIsIntLike(a)) pushInt(-a.lVal);
                else pushDouble(-bcAsDouble(a));
                ++ip; break;
            }

            // ---- Comparison ----
            case OpCode::EQ: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal == b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal == b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::BOOL && b.type == TzdValue::BOOL) {
                        bool res = a.bVal == b.bVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                pushBool(TzdInterpreter::valuesEqual(a, b));
                ++ip; break;
            }
            case OpCode::NE: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal != b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal != b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::BOOL && b.type == TzdValue::BOOL) {
                        bool res = a.bVal != b.bVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                pushBool(!TzdInterpreter::valuesEqual(a, b));
                ++ip; break;
            }
            case OpCode::LT: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal < b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal < b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        bool res = (double)a.lVal < b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        bool res = a.dVal < (double)b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                pushBool(bcAsDouble(a) < bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::LE: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal <= b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal <= b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        bool res = (double)a.lVal <= b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        bool res = a.dVal <= (double)b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                pushBool(bcAsDouble(a) <= bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::GT: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal > b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal > b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        bool res = (double)a.lVal > b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        bool res = a.dVal > (double)b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                pushBool(bcAsDouble(a) > bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::GE: {
                if (m_sp >= stackBase + 2) {
                    TzdValue& a = m_stack[m_sp - 2];
                    TzdValue& b = m_stack[m_sp - 1];
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal >= b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal >= b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::INT && b.type == TzdValue::DOUBLE) {
                        bool res = (double)a.lVal >= b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::INT) {
                        bool res = a.dVal >= (double)b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        --m_sp; ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                pushBool(bcAsDouble(a) >= bcAsDouble(b));
                ++ip; break;
            }

            // ---- Logical ----
            case OpCode::AND: {
                TzdValue b, a; pop(b); pop(a);
                pushBool(TzdInterpreter::isTruthy(a) && TzdInterpreter::isTruthy(b));
                ++ip; break;
            }
            case OpCode::OR: {
                TzdValue b, a; pop(b); pop(a);
                pushBool(TzdInterpreter::isTruthy(a) || TzdInterpreter::isTruthy(b));
                ++ip; break;
            }
            case OpCode::NOT: {
                if (m_sp > stackBase) {
                    TzdValue& a = m_stack[m_sp - 1];
                    if (a.type == TzdValue::BOOL) {
                        a.bVal = !a.bVal;
                        ++ip; break;
                    }
                }
                TzdValue a; pop(a);
                pushBool(!TzdInterpreter::isTruthy(a));
                ++ip; break;
            }

            // ---- Control flow ----
            case OpCode::JMP: {
                ip = (size_t)instr.arg1;
                break;
            }
            case OpCode::JMP_FALSE: {
                bool truthy = false;
                if (m_sp > stackBase) {
                    const TzdValue& top = m_stack[--m_sp];
                    if (top.type == TzdValue::BOOL) truthy = top.bVal;
                    else if (top.type == TzdValue::INT || top.type == TzdValue::LONG) truthy = (top.lVal != 0);
                    else truthy = TzdInterpreter::isTruthy(top);
                }
                ip = truthy ? (ip + 1) : (size_t)instr.arg1;
                break;
            }
            case OpCode::JMP_TRUE: {
                bool truthy = false;
                if (m_sp > stackBase) {
                    const TzdValue& top = m_stack[--m_sp];
                    if (top.type == TzdValue::BOOL) truthy = top.bVal;
                    else if (top.type == TzdValue::INT || top.type == TzdValue::LONG) truthy = (top.lVal != 0);
                    else truthy = TzdInterpreter::isTruthy(top);
                }
                ip = truthy ? (size_t)instr.arg1 : (ip + 1);
                break;
            }

            // ---- Function calls ----
            case OpCode::CALL_FUNC: {
                if (instr.cache >= 0) {
                    if (m_interp && !m_interp->m_noJit) {
                        void* jitPtr = instr.cacheClass;
                        if (!jitPtr) {
                            if (instr.cacheIndex < 0 || ++instr.cacheIndex >= 64) {
                                instr.cacheIndex = 0;
                                const std::string& name = module.constants[instr.arg1].sVal;
                                if (TzdBytecodeJIT::getInstance().getHotThreshold() < 1000000) {
                                    jitPtr = TzdBytecodeJIT::getInstance().onFunctionCall(module, name, m_interp);
                                    if (jitPtr) {
                                        instr.cacheClass = jitPtr;
                                    }
                                }
                            }
                        }
                        if (jitPtr) {
                            const std::string& name = module.constants[instr.arg1].sVal;
                            int argc = instr.arg2;
                            std::vector<TzdValue> callArgs(argc);
                            for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                            TzdValue fv = m_interp->getVariable(name, nullptr);
                            TzdValue r = m_interp->callFunction(fv, callArgs);
                            pushValueMove(std::move(r));
                            ++ip;
                            break;
                        }
                    }

                    if (callFrames.size() >= 10000) {
                        throw std::runtime_error("Maximum call stack size exceeded in bytecode VM");
                    }

                    size_t targetFuncIdx = (size_t)instr.cache;
                    const BytecodeFunc& calleeFunc = module.functions[targetFuncIdx];
                    int calleeLocals = (calleeFunc.localCount > calleeFunc.paramCount) ? calleeFunc.localCount : calleeFunc.paramCount;
                    size_t newLocalBase = m_localTop;
                    size_t neededSlots = newLocalBase + (size_t)calleeLocals + 1;
                    if (m_locals.size() < neededSlots) {
                        m_locals.resize(neededSlots * 2);
                    }
                    m_localTop = neededSlots;

                    int argc = instr.arg2;
                    size_t argsStart = m_sp - argc;
                    for (int i = 0; i < calleeFunc.paramCount && i < argc; ++i) {
                        moveValueFast(m_locals[newLocalBase + i], std::move(m_stack[argsStart + i]));
                    }
                    for (size_t i = newLocalBase + argc; i < newLocalBase + calleeLocals; ++i) {
                        releaseResourceIfNeeded(m_locals[i]);
                        m_locals[i].type = TzdValue::NONE;
                        m_locals[i].lVal = 0;
                    }
                    m_sp = argsStart;

                    callFrames.push_back({
                        currentFuncIdx,
                        code,
                        codeSize,
                        ip + 1,
                        localBase,
                        stackBase,
                        handlerBase
                    });

                    currentFuncIdx = targetFuncIdx;
                    curFunc = &calleeFunc;
                    code = curFunc->code.data();
                    codeSize = curFunc->code.size();
                    ip = 0;
                    localBase = newLocalBase;
                    stackBase = m_sp;
                    handlerBase = m_handlers.size();
                    break;
                }

                if (instr.cache == -3) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                    pushDouble((double)ms);
                    ++ip; break;
                }
                if (instr.cache == -4) {
                    TzdValue val;
                    pop(val);
                    pushValueMove(TzdValue(bcValueToString(val)));
                    ++ip; break;
                }

                const std::string& name = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;

                if (name == "clock" && argc == 0) {
                    instr.cache = -3;
                    auto now = std::chrono::high_resolution_clock::now();
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                    pushDouble((double)ms);
                    ++ip; break;
                }
                if (name == "toString" && argc == 1) {
                    instr.cache = -4;
                    TzdValue val;
                    pop(val);
                    pushValueMove(TzdValue(bcValueToString(val)));
                    ++ip; break;
                }

                auto it = module.funcIndex.find(name);
                if (it != module.funcIndex.end()) {
                    instr.cache = (int32_t)it->second;
                    continue;
                } else {
                    instr.cache = -2;
                }

                if (m_interp) {
                    std::vector<TzdValue> callArgs(argc);
                    for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                    TzdValue fv = m_interp->getVariable(name, nullptr);
                    TzdValue r = callValue(module, fv, callArgs);
                    pushValueMove(std::move(r));
                    ++ip; break;
                } else {
                    throw std::runtime_error("Undefined function: " + name);
                }
            }
            case OpCode::CALL_NATIVE: {
                const std::string& name = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                std::vector<TzdValue> callArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                TzdValue fv = m_interp->getVariable(name, nullptr);
                pushValueMove(callValue(module, fv, callArgs));
                ++ip; break;
            }
            case OpCode::CALL_VALUE: {
                int argc = instr.arg1;
                std::vector<TzdValue> callArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                TzdValue callee; pop(callee);
                pushValueMove(callValue(module, callee, callArgs));
                ++ip; break;
            }
            case OpCode::CALL_METHOD: {
                const std::string& mname = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                std::vector<TzdValue> callArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                TzdValue receiver; pop(receiver);

                TzdSelector msel = tzdInternSelector(mname);
                if (receiver.type == TzdValue::INSTANCE && receiver.instanceVal) {
                    TzdClassDef* cls = receiver.instanceVal->definition;
                    ClassMethod* m = cls ? cls->findMethod(mname) : nullptr;
                    if (m && m->body) {
                        auto it = module.funcIndex.find(m->name);
                        if (it != module.funcIndex.end()) {
                            size_t targetFuncIdx = it->second;
                            const BytecodeFunc& calleeFunc = module.functions[targetFuncIdx];
                            int calleeLocals = (calleeFunc.localCount > calleeFunc.paramCount) ? calleeFunc.localCount : calleeFunc.paramCount;
                            size_t newLocalBase = m_localTop;
                            size_t neededSlots = newLocalBase + (size_t)calleeLocals + 1;
                            if (m_locals.size() < neededSlots) {
                                m_locals.resize(neededSlots * 2);
                            }
                            m_localTop = neededSlots;

                            moveValueFast(m_locals[newLocalBase], std::move(receiver));
                            for (int i = 0; i < argc && i + 1 < calleeFunc.paramCount; ++i) {
                                moveValueFast(m_locals[newLocalBase + 1 + i], std::move(callArgs[i]));
                            }
                            for (size_t i = newLocalBase + 1 + argc; i < newLocalBase + calleeLocals; ++i) {
                                releaseResourceIfNeeded(m_locals[i]);
                                m_locals[i].type = TzdValue::NONE;
                                m_locals[i].lVal = 0;
                            }

                            callFrames.push_back({
                                currentFuncIdx,
                                code,
                                codeSize,
                                ip + 1,
                                localBase,
                                stackBase,
                                handlerBase
                            });

                            currentFuncIdx = targetFuncIdx;
                            curFunc = &calleeFunc;
                            code = curFunc->code.data();
                            codeSize = curFunc->code.size();
                            ip = 0;
                            localBase = newLocalBase;
                            stackBase = m_sp;
                            handlerBase = m_handlers.size();
                            break;
                        }
                    } else if (receiver.type == TzdValue::CLASS_DEF && receiver.classDefVal) {
                        const TzdMemberSlot* slot = receiver.classDefVal->tzdDispatch.find(msel);
                        ClassMethod* sm = (slot && slot->method) ? slot->method : receiver.classDefVal->findMethod(mname);
                        if (sm) {
                            if (receiver.classDefVal && sm->body && m_interp && !m_interp->m_noJit) {
                                std::string fullMName = receiver.classDefVal->fullName + "_" + mname;
                                if (!sm->jittedPtr) {
                                    void* jitPtr = TzdBytecodeJIT::getInstance().onFunctionCall(module, fullMName, m_interp);
                                    if (jitPtr) {
                                        sm->jittedPtr = reinterpret_cast<void(*)(void*,void*)>(jitPtr);
                                    }
                                }
                            }
                            TzdValue bound;
                            if (sm->templateVal) {
                                bound = *(sm->templateVal);
                            } else {
                                bound = TzdValue(sm->name, sm->params, sm->body);
                                bound.paramTypes = sm->paramTypes;
                            }
                            bound.jittedPtr = sm->jittedPtr;
                            pushValueMove(callValue(module, bound, callArgs));
                            ++ip; break;
                        }
                    } else if (receiver.type == TzdValue::MAP) {
                        auto it = receiver.mapVal.find(mname);
                        if (it != receiver.mapVal.end()) {
                            pushValueMove(callValue(module, it->second, callArgs));
                            ++ip; break;
                        }
                    }
                }
                pushValueMove(callValue(module, TzdValue(), callArgs));
                ++ip; break;
            }

            // ---- Returns ----
            case OpCode::RET: {
                if (!callFrames.empty()) {
                    bool hasRet = (m_sp > stackBase);
                    size_t retIdx = m_sp - 1;

                    for (size_t i = localBase; i < m_localTop; ++i) {
                        releaseResourceIfNeeded(m_locals[i]);
                        m_locals[i].type = TzdValue::NONE;
                    }
                    m_localTop = localBase;

                    if (m_handlers.size() > handlerBase) {
                        m_handlers.resize(handlerBase);
                    }

                    const VMCallFrame& prev = callFrames.back();
                    currentFuncIdx = prev.funcIndex;
                    curFunc = &module.functions[currentFuncIdx];
                    code = prev.code;
                    codeSize = prev.codeSize;
                    ip = prev.ip;
                    localBase = prev.localBase;
                    size_t calleeStackBase = stackBase;
                    stackBase = prev.stackBase;
                    handlerBase = prev.handlerBase;
                    callFrames.pop_back();

                    if (hasRet) {
                        if (retIdx != calleeStackBase) {
                            moveValueFast(m_stack[calleeStackBase], std::move(m_stack[retIdx]));
                        }
                        m_sp = calleeStackBase + 1;
                    } else {
                        m_sp = calleeStackBase;
                        pushNull();
                    }
                    break;
                } else {
                    if (m_sp > stackBase) {
                        moveValueFast(result, std::move(m_stack[--m_sp]));
                    } else {
                        result = TzdValue();
                    }
                    goto funcExit;
                }
            }
            case OpCode::RET_VOID: {
                for (size_t i = localBase; i < m_localTop; ++i) {
                    releaseResourceIfNeeded(m_locals[i]);
                    m_locals[i].type = TzdValue::NONE;
                }
                m_localTop = localBase;

                if (m_handlers.size() > handlerBase) {
                    m_handlers.resize(handlerBase);
                }

                if (!callFrames.empty()) {
                    const VMCallFrame& prev = callFrames.back();
                    currentFuncIdx = prev.funcIndex;
                    curFunc = &module.functions[currentFuncIdx];
                    code = prev.code;
                    codeSize = prev.codeSize;
                    ip = prev.ip;
                    localBase = prev.localBase;
                    size_t calleeStackBase = stackBase;
                    stackBase = prev.stackBase;
                    handlerBase = prev.handlerBase;
                    callFrames.pop_back();

                    m_sp = calleeStackBase;
                    pushNull();
                    break;
                } else {
                    result = TzdValue();
                    goto funcExit;
                }
            }

            // ---- Stack ops ----
            case OpCode::POP: {
                if (m_sp > stackBase) --m_sp;
                ++ip; break;
            }
            case OpCode::DUP: {
                if (m_sp > stackBase) {
                    const TzdValue& top = m_stack[m_sp - 1];
                    pushValue(top);
                }
                ++ip; break;
            }

            // ---- Other ----
            case OpCode::PRINT: {
                int argc = instr.arg1;
                std::vector<TzdValue> parts(argc);
                for (int i = argc - 1; i >= 0; --i) pop(parts[i]);
                for (int i = 0; i < argc; ++i) {
                    std::cout << bcValueToString(parts[i]);
                    if (i < argc - 1) std::cout << " ";
                }
                std::cout << std::endl;
                pushNull();
                ++ip; break;
            }
            case OpCode::THROW: {
                TzdValue v; pop(v);
                throw TzdThrowException(std::move(v));
            }
            case OpCode::SCOPE_PUSH: case OpCode::SCOPE_POP: {
                ++ip; break;
            }

            // ---- OOP / Container ----
            case OpCode::LOAD_MEMBER: {
                const std::string& name = module.constants[instr.arg1].sVal;
                TzdValue obj; pop(obj);
                if (obj.type == TzdValue::INSTANCE && obj.instanceVal) {
                    TzdClassDef* cls = obj.instanceVal->definition;
                    if (instr.cacheClass == (void*)cls && instr.cacheIndex >= 0) {
                        const TzdValue& val = obj.instanceVal->fieldValues[instr.cacheIndex];
                        switch (val.type) {
                            case TzdValue::DOUBLE: case TzdValue::FLOAT: pushDouble(val.dVal); break;
                            case TzdValue::INT: case TzdValue::LONG: pushInt(val.lVal); break;
                            case TzdValue::BOOL: pushBool(val.bVal); break;
                            default: pushValue(val); break;
                        }
                        ++ip; break;
                    }
                    int fieldIdx = cls->getFieldIndex(name);
                    if (fieldIdx >= 0) {
                        instr.cacheClass = (void*)cls;
                        instr.cacheIndex = fieldIdx;
                        const TzdValue& val = obj.instanceVal->fieldValues[fieldIdx];
                        switch (val.type) {
                            case TzdValue::DOUBLE: case TzdValue::FLOAT: pushDouble(val.dVal); break;
                            case TzdValue::INT: case TzdValue::LONG: pushInt(val.lVal); break;
                            case TzdValue::BOOL: pushBool(val.bVal); break;
                            default: pushValue(val); break;
                        }
                    } else {
                        TzdSelector sel = tzdInternSelector(name);
                        pushValue(obj.instanceVal->getMember(sel, name));
                    }
                } else {
                    pushValue(getMember(obj, name));
                }
                ++ip; break;
            }
            case OpCode::STORE_MEMBER: {
                const std::string& name = module.constants[instr.arg1].sVal;
                TzdValue val; pop(val);
                TzdValue obj; pop(obj);
                if (obj.type == TzdValue::INSTANCE && obj.instanceVal) {
                    TzdClassDef* cls = obj.instanceVal->definition;
                    if (instr.cacheClass == (void*)cls && instr.cacheIndex >= 0) {
                        obj.instanceVal->fieldValues[instr.cacheIndex] = val;
                        pushValue(val);
                        ++ip; break;
                    }
                    int fieldIdx = cls->getFieldIndex(name);
                    if (fieldIdx >= 0) {
                        instr.cacheClass = (void*)cls;
                        instr.cacheIndex = fieldIdx;
                        obj.instanceVal->fieldValues[fieldIdx] = val;
                    } else {
                        TzdSelector sel = tzdInternSelector(name);
                        obj.instanceVal->setMember(sel, name, val);
                    }
                } else {
                    setMember(obj, name, val);
                }
                pushValue(val);
                ++ip; break;
            }
            case OpCode::LOAD_INDEX: {
                TzdValue idx; pop(idx);
                TzdValue container; pop(container);
                int i = (idx.type == TzdValue::DOUBLE) ? (int)idx.dVal : (int)idx.lVal;
                if (container.type == TzdValue::ARRAY) {
                    if (i < 0 || i >= (int)container.arrVal.size())
                        throw std::runtime_error("Array index out of bounds: " + std::to_string(i));
                    pushValue(container.arrVal[i]);
                } else if (container.type == TzdValue::STRING) {
                    if (i < 0 || i >= (int)container.sVal.size())
                        throw std::runtime_error("String index out of bounds: " + std::to_string(i));
                    pushValueMove(TzdValue(std::string(1, container.sVal[i])));
                } else if (container.type == TzdValue::MAP) {
                    std::string key = (idx.type == TzdValue::STRING) ? idx.sVal : TzdInterpreter::getAsString(std::any(idx));
                    auto it = container.mapVal.find(key);
                    pushValue(it != container.mapVal.end() ? it->second : TzdValue());
                } else {
                    throw std::runtime_error("Type does not support indexing");
                }
                ++ip; break;
            }
            case OpCode::STORE_INDEX: {
                TzdValue val; pop(val);
                TzdValue idx; pop(idx);
                TzdValue container; pop(container);
                int i = (idx.type == TzdValue::DOUBLE) ? (int)idx.dVal : (int)idx.lVal;
                if (container.type == TzdValue::ARRAY) {
                    if (i < 0) i = (int)container.arrVal.size();
                    if (i >= (int)container.arrVal.size())
                        container.arrVal.resize(i + 1);
                    container.arrVal[i] = val;
                } else if (container.type == TzdValue::MAP) {
                    std::string key = (idx.type == TzdValue::STRING) ? idx.sVal : TzdInterpreter::getAsString(std::any(idx));
                    container.mapVal[key] = val;
                }
                pushValue(val);
                pushValueMove(std::move(container));
                ++ip; break;
            }
            case OpCode::NEW_OBJECT: {
                const std::string& className = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                std::vector<TzdValue> ctorArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(ctorArgs[i]);

                TzdClassDef* cls = TzdOopManager::getClass(className);
                if (!cls)
                    throw std::runtime_error("Cannot find class definition: " + className);
                TzdInstance* inst = new TzdInstance(cls);
                TzdValue instVal(inst);

                ClassConstructor* ctor = cls->findConstructor(argc);
                if (!ctor && argc == 0 && !cls->constructors.empty())
                    ctor = cls->findConstructor(0);

                if (ctor) {
                    if (ctor->body && m_interp && !m_interp->m_noJit && !ctor->jittedPtr) {
                        std::string ctorJitName = cls->fullName + "_" + cls->simpleName;
                        void* jitPtr = TzdBytecodeJIT::getInstance().onFunctionCall(module, ctorJitName, m_interp);
                        if (jitPtr) {
                            ctor->jittedPtr = reinterpret_cast<void(*)(void*,void*)>(jitPtr);
                        }
                    }
                    std::unordered_map<std::string, TzdValue> ctorScope;
                    ctorScope["this"] = instVal;
                    m_interp->scopes.push_back(std::move(ctorScope));
                    try {
                        TzdValue ctorFunc(cls->simpleName, ctor->params, ctor->body);
                        ctorFunc.jittedPtr = ctor->jittedPtr;
                        ctorFunc.setInstance(inst);
                        m_interp->callFunction(ctorFunc, ctorArgs);
                    } catch (...) {
                        m_interp->scopes.pop_back();
                        throw;
                    }
                    m_interp->scopes.pop_back();
                }
                pushValueMove(std::move(instVal));
                ++ip; break;
            }
            case OpCode::MAKE_ARRAY: {
                int count = instr.arg1;
                std::vector<TzdValue> elems(count);
                for (int i = count - 1; i >= 0; --i) pop(elems[i]);
                pushValueMove(TzdValue(elems));
                ++ip; break;
            }
            case OpCode::TRY: {
                m_handlers.push_back({currentFuncIdx, (size_t)instr.arg1, m_sp, localBase, ""});
                ++ip; break;
            }
            case OpCode::TRY_END: {
                if (!m_handlers.empty()) m_handlers.pop_back();
                ++ip; break;
            }
            case OpCode::HALT: {
                goto funcExit;
            }
            default:
                ++ip; break;
        }
    } // end while (ip < codeSize)

    // Reached end of function without explicit RET/RET_VOID
    if (!callFrames.empty()) {
        for (size_t i = localBase; i < m_localTop; ++i) {
            releaseResourceIfNeeded(m_locals[i]);
            m_locals[i].type = TzdValue::NONE;
        }
        m_localTop = localBase;
        m_sp = stackBase;
        if (m_handlers.size() > handlerBase) {
            m_handlers.resize(handlerBase);
        }
        const VMCallFrame& prev = callFrames.back();
        currentFuncIdx = prev.funcIndex;
        curFunc = &module.functions[currentFuncIdx];
        code = prev.code;
        codeSize = prev.codeSize;
        ip = prev.ip;
        localBase = prev.localBase;
        stackBase = prev.stackBase;
        handlerBase = prev.handlerBase;
        callFrames.pop_back();

        pushNull();
        continue;
    }
    break; // normal exit
    } // end try
    catch (const TzdThrowException& ex) {
        bool handled = false;
        while (!m_handlers.empty()) {
            CatchFrame cf = m_handlers.back();
            m_handlers.pop_back();
            if (cf.funcIndex == currentFuncIdx) {
                m_sp = cf.stackBase;
                pushValue(ex.value);
                ip = cf.catchIp;
                handled = true;
                break;
            } else {
                bool foundInStack = false;
                for (auto it = callFrames.rbegin(); it != callFrames.rend(); ++it) {
                    if (it->funcIndex == cf.funcIndex) {
                        foundInStack = true;
                        break;
                    }
                }
                if (foundInStack) {
                    while (!callFrames.empty() && currentFuncIdx != cf.funcIndex) {
                        for (size_t i = localBase; i < m_localTop; ++i) {
                            releaseResourceIfNeeded(m_locals[i]);
                            m_locals[i].type = TzdValue::NONE;
                        }
                        m_localTop = localBase;
                        const VMCallFrame& prev = callFrames.back();
                        currentFuncIdx = prev.funcIndex;
                        curFunc = &module.functions[currentFuncIdx];
                        code = prev.code;
                        codeSize = prev.codeSize;
                        localBase = prev.localBase;
                        stackBase = prev.stackBase;
                        handlerBase = prev.handlerBase;
                        callFrames.pop_back();
                    }
                    m_sp = cf.stackBase;
                    pushValue(ex.value);
                    ip = cf.catchIp;
                    handled = true;
                    break;
                }
            }
        }
        if (!handled) {
            while (!callFrames.empty()) {
                const VMCallFrame& prev = callFrames.back();
                localBase = prev.localBase;
                stackBase = prev.stackBase;
                handlerBase = prev.handlerBase;
                callFrames.pop_back();
            }
            for (size_t i = initialLocalBase; i < m_localTop; ++i) {
                releaseResourceIfNeeded(m_locals[i]);
                m_locals[i].type = TzdValue::NONE;
            }
            m_localTop = initialLocalBase;
            m_sp = initialStackBase;
            m_handlers.resize(initialHandlerBase);
            throw;
        }
        continue;
    }
    catch (...) {
        while (!callFrames.empty()) {
            const VMCallFrame& prev = callFrames.back();
            localBase = prev.localBase;
            stackBase = prev.stackBase;
            handlerBase = prev.handlerBase;
            callFrames.pop_back();
        }
        for (size_t i = initialLocalBase; i < m_localTop; ++i) {
            releaseResourceIfNeeded(m_locals[i]);
            m_locals[i].type = TzdValue::NONE;
        }
        m_localTop = initialLocalBase;
        m_sp = initialStackBase;
        m_handlers.resize(initialHandlerBase);
        throw;
    }
    } // end for(;;)

funcExit:
    while (!callFrames.empty()) {
        const VMCallFrame& prev = callFrames.back();
        localBase = prev.localBase;
        stackBase = prev.stackBase;
        handlerBase = prev.handlerBase;
        callFrames.pop_back();
    }
    for (size_t i = initialLocalBase; i < m_localTop; ++i) {
        releaseResourceIfNeeded(m_locals[i]);
        m_locals[i].type = TzdValue::NONE;
    }
    m_localTop = initialLocalBase;
    m_sp = initialStackBase;
    m_handlers.resize(initialHandlerBase);
    return result;
}
