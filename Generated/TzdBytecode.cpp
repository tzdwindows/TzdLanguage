// ============================================================================
// TzdBytecode.cpp - Bytecode VM compiler and virtual machine
// Full implementation: AST -> Bytecode compiler (ANTLR visitor) + stack VM.
// ============================================================================

#include "TzdBytecode.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

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

void TzdBytecodeCompiler::emit(OpCode op, int32_t arg1, int32_t arg2) {
    if (!m_currentFunc) return;
    Instruction instr;
    instr.op = op;
    instr.arg1 = arg1;
    instr.arg2 = arg2;
    m_currentFunc->code.push_back(instr);
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
    return op == OpCode::LOAD_LOCAL || op == OpCode::STORE_LOCAL;
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
    int64_t val = 0;
    try {
        val = std::stoll(raw, nullptr, 0);
    } catch (...) {
        try { val = (int64_t)std::stod(raw); } catch (...) { val = 0; }
    }
    emit(OpCode::PUSH_INT, addConstant(val));
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
            if (slot >= 0) emit(OpCode::LOAD_LOCAL, slot);
            else emit(OpCode::LOAD_VAR, addConstant(name));
            emit(OpCode::PUSH_INT, addConstant((int64_t)1));
            emit(isInc ? OpCode::ADD : OpCode::SUB);
            emit(OpCode::DUP);
            if (slot >= 0) emit(OpCode::STORE_LOCAL, slot);
            else emit(OpCode::STORE_VAR, addConstant(name));
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
            if (slot >= 0) emit(OpCode::LOAD_LOCAL, slot);
            else emit(OpCode::LOAD_VAR, addConstant(name));
            emit(OpCode::DUP);                   // keep old as result
            emit(OpCode::PUSH_INT, addConstant((int64_t)1));
            emit(isInc ? OpCode::ADD : OpCode::SUB);
            if (slot >= 0) emit(OpCode::STORE_LOCAL, slot);
            else emit(OpCode::STORE_VAR, addConstant(name));
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
    m_stack.reserve(4096);  // pre-allocate to avoid reallocation during execution
    m_locals.reserve(1024);
}

TzdValue TzdBytecodeVM::execute(const BytecodeModule& module) {
    if (module.functions.empty()) return TzdValue();
    m_module = &module;
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

TzdValue TzdBytecodeVM::runBytecodeFunc(const BytecodeModule& module,
                                        size_t funcIndex,
                                        const TzdValue* argsData,
                                        size_t argCount) {
    if (funcIndex >= module.functions.size()) return TzdValue();
    const BytecodeFunc& f = module.functions[funcIndex];
    const std::vector<Instruction>& code = f.code;

    // Fresh local frame (locals are relative to localBase).
    size_t localBase = m_locals.size();
    int localsNeeded = (f.localCount > f.paramCount) ? f.localCount : f.paramCount;
    m_locals.resize(localBase + (size_t)localsNeeded + 1);

    // Pre-allocate operand stack space based on verifier-computed maxStackDepth
    if (f.maxStackDepth > 0) {
        m_stack.reserve(m_stack.size() + (size_t)f.maxStackDepth + 4);
    }

    for (int i = 0; i < f.paramCount && i < (int)argCount; ++i) {
        m_locals[localBase + i] = argsData[i];
    }

    size_t stackBase = m_stack.size();
    size_t handlerBase = m_handlers.size();
    size_t ip = 0;
    TzdValue result; // default null

    auto pop = [&](TzdValue& out) -> bool {
        if (m_stack.size() <= stackBase) return false;
        out = std::move(m_stack.back());
        m_stack.pop_back();
        return true;
    };

    // Main execution loop: try/catch is OUTSIDE the while to avoid
    // per-instruction exception handling overhead. The outer for(;;)
    // re-enters the loop only when an exception is caught.
    for (;;) {
    try {
    while (ip < code.size()) {
        const Instruction& instr = code[ip];
        switch (instr.op) {
            // ---- Constants ----
            case OpCode::PUSH_DOUBLE: {
                const ConstEntry& c = module.constants[instr.arg1];
                m_stack.emplace_back(c.dVal);
                ++ip; break;
            }
            case OpCode::PUSH_INT: {
                const ConstEntry& c = module.constants[instr.arg1];
                m_stack.emplace_back(c.iVal);
                ++ip; break;
            }
            case OpCode::PUSH_STRING: {
                const ConstEntry& c = module.constants[instr.arg1];
                m_stack.emplace_back(c.sVal);
                ++ip; break;
            }
            case OpCode::PUSH_BOOL: {
                const ConstEntry& c = module.constants[instr.arg1];
                m_stack.emplace_back(c.bVal);
                ++ip; break;
            }
            case OpCode::PUSH_NULL: {
                m_stack.emplace_back();
                ++ip; break;
            }

            // ---- Variables ----
            case OpCode::LOAD_LOCAL: {
                m_stack.push_back(m_locals[localBase + instr.arg1]);
                ++ip; break;
            }
            case OpCode::STORE_LOCAL: {
                m_locals[localBase + instr.arg1] = std::move(m_stack.back());
                m_stack.pop_back();
                ++ip; break;
            }
            case OpCode::LOAD_VAR: {
                const std::string& name = module.constants[instr.arg1].sVal;
                auto it = module.funcIndex.find(name);
                if (it != module.funcIndex.end()) {
                    TzdValue fv;
                    fv.type = TzdValue::FUNCTION;
                    fv.name = name; // bytecode sentinel (funcBody null)
                    m_stack.push_back(std::move(fv));
                } else {
                    m_stack.push_back(m_interp->getVariable(name, nullptr));
                }
                ++ip; break;
            }
            case OpCode::STORE_VAR: {
                const std::string& name = module.constants[instr.arg1].sVal;
                m_interp->setVariable(name, m_stack.back());
                m_stack.pop_back();
                ++ip; break;
            }

            // ---- Arithmetic ----
            case OpCode::ADD: {
                if (m_stack.size() >= stackBase + 2) {
                    auto& b = m_stack.back();
                    auto& a = *(m_stack.end() - 2);
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        a.lVal += b.lVal;
                        m_stack.pop_back();
                        ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        a.dVal += b.dVal;
                        m_stack.pop_back();
                        ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (a.type == TzdValue::STRING || b.type == TzdValue::STRING) {
                    m_stack.emplace_back(bcValueToString(a) + bcValueToString(b));
                } else if (bcIsIntLike(a) && bcIsIntLike(b)) {
                    m_stack.emplace_back(a.lVal + b.lVal);
                } else {
                    m_stack.emplace_back(bcAsDouble(a) + bcAsDouble(b));
                }
                ++ip; break;
            }
            case OpCode::SUB: {
                if (m_stack.size() >= stackBase + 2) {
                    auto& b = m_stack.back();
                    auto& a = *(m_stack.end() - 2);
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        a.lVal -= b.lVal;
                        m_stack.pop_back();
                        ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        a.dVal -= b.dVal;
                        m_stack.pop_back();
                        ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b))
                    m_stack.emplace_back(a.lVal - b.lVal);
                else
                    m_stack.emplace_back(bcAsDouble(a) - bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::MUL: {
                if (m_stack.size() >= stackBase + 2) {
                    auto& b = m_stack.back();
                    auto& a = *(m_stack.end() - 2);
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        a.lVal *= b.lVal;
                        m_stack.pop_back();
                        ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        a.dVal *= b.dVal;
                        m_stack.pop_back();
                        ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b))
                    m_stack.emplace_back(a.lVal * b.lVal);
                else
                    m_stack.emplace_back(bcAsDouble(a) * bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::DIV: {
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b)) {
                    if (b.lVal == 0) throw std::runtime_error("Division by zero");
                    m_stack.emplace_back(a.lVal / b.lVal);
                } else {
                    double r = bcAsDouble(b);
                    if (r == 0.0) throw std::runtime_error("Division by zero");
                    m_stack.emplace_back(bcAsDouble(a) / r);
                }
                ++ip; break;
            }
            case OpCode::MOD: {
                TzdValue b, a; pop(b); pop(a);
                if (bcIsIntLike(a) && bcIsIntLike(b) && b.lVal != 0) {
                    m_stack.emplace_back(a.lVal % b.lVal);
                } else {
                    double r = bcAsDouble(b);
                    if (r == 0.0) throw std::runtime_error("Modulo by zero");
                    m_stack.emplace_back(std::fmod(bcAsDouble(a), r));
                }
                ++ip; break;
            }
            case OpCode::POW: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(std::pow(bcAsDouble(a), bcAsDouble(b)));
                ++ip; break;
            }
            case OpCode::NEG: {
                TzdValue a; pop(a);
                if (bcIsIntLike(a)) m_stack.emplace_back(-a.lVal);
                else m_stack.emplace_back(-bcAsDouble(a));
                ++ip; break;
            }

            // ---- Comparison ----
            case OpCode::EQ: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(TzdInterpreter::valuesEqual(a, b));
                ++ip; break;
            }
            case OpCode::NE: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(!TzdInterpreter::valuesEqual(a, b));
                ++ip; break;
            }
            case OpCode::LT: {
                if (m_stack.size() >= stackBase + 2) {
                    auto& b = m_stack.back();
                    auto& a = *(m_stack.end() - 2);
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal < b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        m_stack.pop_back();
                        ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal < b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        m_stack.pop_back();
                        ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(bcAsDouble(a) < bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::LE: {
                if (m_stack.size() >= stackBase + 2) {
                    auto& b = m_stack.back();
                    auto& a = *(m_stack.end() - 2);
                    if (a.type == TzdValue::INT && b.type == TzdValue::INT) {
                        bool res = a.lVal <= b.lVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        m_stack.pop_back();
                        ++ip; break;
                    } else if (a.type == TzdValue::DOUBLE && b.type == TzdValue::DOUBLE) {
                        bool res = a.dVal <= b.dVal;
                        a.type = TzdValue::BOOL;
                        a.bVal = res;
                        m_stack.pop_back();
                        ++ip; break;
                    }
                }
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(bcAsDouble(a) <= bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::GT: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(bcAsDouble(a) > bcAsDouble(b));
                ++ip; break;
            }
            case OpCode::GE: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(bcAsDouble(a) >= bcAsDouble(b));
                ++ip; break;
            }

            // ---- Logical ----
            case OpCode::AND: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(TzdInterpreter::isTruthy(a) &&
                                     TzdInterpreter::isTruthy(b));
                ++ip; break;
            }
            case OpCode::OR: {
                TzdValue b, a; pop(b); pop(a);
                m_stack.emplace_back(TzdInterpreter::isTruthy(a) ||
                                     TzdInterpreter::isTruthy(b));
                ++ip; break;
            }
            case OpCode::NOT: {
                TzdValue a; pop(a);
                m_stack.emplace_back(!TzdInterpreter::isTruthy(a));
                ++ip; break;
            }

            // ---- Control flow ----
            case OpCode::JMP: {
                ip = (size_t)instr.arg1;
                break;
            }
            case OpCode::JMP_FALSE: {
                TzdValue v; pop(v);
                ip = TzdInterpreter::isTruthy(v) ? (ip + 1) : (size_t)instr.arg1;
                break;
            }
            case OpCode::JMP_TRUE: {
                TzdValue v; pop(v);
                ip = TzdInterpreter::isTruthy(v) ? (size_t)instr.arg1 : (ip + 1);
                break;
            }

            // ---- Function calls ----
            case OpCode::CALL_FUNC: {
                const std::string& name = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                auto it = module.funcIndex.find(name);
                TzdValue r;
                if (it != module.funcIndex.end()) {
                    if ((int)m_stack.size() - (int)stackBase >= argc) {
                        size_t argsStart = m_stack.size() - argc;
                        r = runBytecodeFunc(module, it->second, m_stack.data() + argsStart, (size_t)argc);
                        m_stack.resize(argsStart);
                    } else {
                        std::vector<TzdValue> callArgs(argc);
                        for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                        r = runBytecodeFunc(module, it->second, callArgs.data(), callArgs.size());
                    }
                } else if (m_interp) {
                    std::vector<TzdValue> callArgs(argc);
                    for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                    TzdValue fv = m_interp->getVariable(name, nullptr);
                    r = callValue(module, fv, callArgs);
                }
                m_stack.push_back(std::move(r));
                ++ip; break;
            }
            case OpCode::CALL_NATIVE: {
                const std::string& name = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                std::vector<TzdValue> callArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                TzdValue fv = m_interp->getVariable(name, nullptr);
                m_stack.push_back(callValue(module, fv, callArgs));
                ++ip; break;
            }
            case OpCode::CALL_VALUE: {
                int argc = instr.arg1;
                std::vector<TzdValue> callArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                TzdValue callee; pop(callee);
                m_stack.push_back(callValue(module, callee, callArgs));
                ++ip; break;
            }
            case OpCode::CALL_METHOD: {
                const std::string& mname = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                std::vector<TzdValue> callArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(callArgs[i]);
                TzdValue receiver; pop(receiver);

                TzdValue bound;
                TzdSelector msel = tzdInternSelector(mname);
                if (receiver.type == TzdValue::INSTANCE && receiver.instanceVal) {
                    TzdClassDef* cls = receiver.instanceVal->definition;
                    const TzdMemberSlot* slot = cls ? cls->tzdDispatch.find(msel) : nullptr;
                    ClassMethod* m = (slot && slot->method) ? slot->method : (cls ? cls->findMethod(mname) : nullptr);
                    if (m) {
                        if (m->templateVal) {
                            bound = *(m->templateVal);
                        } else {
                            bound = TzdValue(m->name, m->params, m->body);
                            bound.paramTypes = m->paramTypes;
                        }
                        bound.jittedPtr = m->jittedPtr;
                        bound.setInstance(receiver.instanceVal);
                    } else {
                        bound = receiver.instanceVal->getMember(msel, mname);
                    }
                } else if (receiver.type == TzdValue::CLASS_DEF &&
                           receiver.classDefVal) {
                    const TzdMemberSlot* slot = receiver.classDefVal->tzdDispatch.find(msel);
                    ClassMethod* m = (slot && slot->method) ? slot->method : receiver.classDefVal->findMethod(mname);
                    if (m) {
                        if (m->templateVal) {
                            bound = *(m->templateVal);
                        } else {
                            bound = TzdValue(m->name, m->params, m->body);
                            bound.paramTypes = m->paramTypes;
                        }
                        bound.jittedPtr = m->jittedPtr;
                    }
                } else if (receiver.type == TzdValue::MAP) {
                    auto it = receiver.mapVal.find(mname);
                    if (it != receiver.mapVal.end()) bound = it->second;
                }
                m_stack.push_back(callValue(module, bound, callArgs));
                ++ip; break;
            }

            case OpCode::RET: {
                TzdValue v; pop(v);
                result = std::move(v);
                goto funcExit;
            }
            case OpCode::RET_VOID: {
                goto funcExit;
            }

            // ---- Stack ops ----
            case OpCode::POP: {
                if (m_stack.size() > stackBase) m_stack.pop_back();
                ++ip; break;
            }
            case OpCode::DUP: {
                if (!m_stack.empty()) m_stack.push_back(m_stack.back());
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
                m_stack.emplace_back(); // print returns null
                ++ip; break;
            }
            case OpCode::THROW: {
                TzdValue v; pop(v);
                throw TzdThrowException(std::move(v));
            }
            case OpCode::SCOPE_PUSH: {
                ++ip; break; // no-op: locals are statically allocated
            }
            case OpCode::SCOPE_POP: {
                ++ip; break;
            }

            // ---- OOP / container ----
            case OpCode::LOAD_MEMBER: {
                const std::string& name = module.constants[instr.arg1].sVal;
                TzdValue obj; pop(obj);
                m_stack.push_back(getMember(obj, name));
                ++ip; break;
            }
            case OpCode::STORE_MEMBER: {
                const std::string& name = module.constants[instr.arg1].sVal;
                TzdValue val; pop(val);
                TzdValue obj; pop(obj);
                setMember(obj, name, val);
                m_stack.push_back(std::move(val)); // assignment yields the value
                ++ip; break;
            }
            case OpCode::LOAD_INDEX: {
                TzdValue idx; pop(idx);
                TzdValue container; pop(container);
                int i = (idx.type == TzdValue::DOUBLE) ? (int)idx.dVal
                                                        : (int)idx.lVal;
                if (container.type == TzdValue::ARRAY) {
                    if (i < 0 || i >= (int)container.arrVal.size())
                        throw std::runtime_error("Array index out of bounds: " +
                                                 std::to_string(i));
                    m_stack.push_back(container.arrVal[i]);
                } else if (container.type == TzdValue::STRING) {
                    if (i < 0 || i >= (int)container.sVal.size())
                        throw std::runtime_error("String index out of bounds: " +
                                                 std::to_string(i));
                    m_stack.emplace_back(std::string(1, container.sVal[i]));
                } else if (container.type == TzdValue::MAP) {
                    std::string key;
                    if (idx.type == TzdValue::STRING) key = idx.sVal;
                    else key = TzdInterpreter::getAsString(std::any(idx));
                    auto it = container.mapVal.find(key);
                    m_stack.push_back(it != container.mapVal.end() ? it->second
                                                                   : TzdValue());
                } else {
                    throw std::runtime_error("Type does not support indexing");
                }
                ++ip; break;
            }
            case OpCode::STORE_INDEX: {
                TzdValue val; pop(val);
                TzdValue idx; pop(idx);
                TzdValue container; pop(container);
                int i = (idx.type == TzdValue::DOUBLE) ? (int)idx.dVal
                                                        : (int)idx.lVal;
                if (container.type == TzdValue::ARRAY) {
                    if (i < 0) i = (int)container.arrVal.size();
                    if (i >= (int)container.arrVal.size())
                        container.arrVal.resize(i + 1);
                    container.arrVal[i] = val;
                } else if (container.type == TzdValue::MAP) {
                    // Direct string key access (avoid bcValueToString overhead)
                    std::string key;
                    if (idx.type == TzdValue::STRING) key = idx.sVal;
                    else key = TzdInterpreter::getAsString(std::any(idx));
                    container.mapVal[key] = val;
                }
                // Push value (result) and modified container (for store-back)
                m_stack.push_back(std::move(val));
                m_stack.push_back(std::move(container));
                ++ip; break;
            }
            case OpCode::NEW_OBJECT: {
                const std::string& className = module.constants[instr.arg1].sVal;
                int argc = instr.arg2;
                std::vector<TzdValue> ctorArgs(argc);
                for (int i = argc - 1; i >= 0; --i) pop(ctorArgs[i]);

                TzdClassDef* cls = TzdOopManager::getClass(className);
                if (!cls)
                    throw std::runtime_error("Cannot find class definition: " +
                                             className);
                TzdInstance* inst = new TzdInstance(cls);
                TzdValue instVal(inst);

                std::unordered_map<std::string, TzdValue> ctorScope;
                ctorScope["this"] = instVal;
                m_interp->scopes.push_back(std::move(ctorScope));
                try {
                    // Run field initializers via the interpreter (AST eval).
                    TzdClassDef* cur = cls;
                    std::vector<TzdClassDef*> hierarchy;
                    while (cur) {
                        hierarchy.insert(hierarchy.begin(), cur);
                        if (cur->parentName.empty()) break;
                        cur = TzdOopManager::getClass(cur->parentName);
                    }
                    for (auto* ccls : hierarchy) {
                        for (auto const& fnameField : ccls->fields) {
                            const std::string& fname = fnameField.first;
                            const ClassField& field = fnameField.second;
                            if (!field.isStatic && field.initExpr) {
                                std::any iv = m_interp->visit(field.initExpr);
                                TzdValue initVal;
                                if (iv.has_value() &&
                                    iv.type() == typeid(TzdValue))
                                    initVal = std::any_cast<TzdValue>(iv);
                                for (auto const& pr : cls->fieldIndices) {
                                    if (pr.first == fname) {
                                        inst->fieldValues[pr.second] = initVal;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    // Run matching constructor via the interpreter.
                    ClassConstructor* ctor = cls->findConstructor(argc);
                    if (!ctor && argc == 0 && !cls->constructors.empty())
                        ctor = cls->findConstructor(0);
                    if (ctor) {
                        TzdValue ctorFunc(cls->simpleName, ctor->params,
                                           ctor->body);
                        ctorFunc.jittedPtr = ctor->jittedPtr;
                        ctorFunc.setInstance(inst);
                        m_interp->callFunction(ctorFunc, ctorArgs);
                    }
                } catch (...) {
                    m_interp->scopes.pop_back();
                    throw;
                }
                m_interp->scopes.pop_back();
                m_stack.push_back(std::move(instVal));
                ++ip; break;
            }
            case OpCode::MAKE_ARRAY: {
                int count = instr.arg1;
                std::vector<TzdValue> elems(count);
                for (int i = count - 1; i >= 0; --i) pop(elems[i]);
                m_stack.emplace_back(elems);
                ++ip; break;
            }

            // ---- Exception handling ----
            case OpCode::TRY: {
                m_handlers.push_back({funcIndex, (size_t)instr.arg1,
                                      stackBase, localBase, ""});
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
        } // end while
        break; // normal exit from while loop → break out of for(;;)
    } // end try
    catch (const TzdThrowException& ex) {
            // Unwind to the nearest active handler registered in this frame.
            bool handled = false;
            while (!m_handlers.empty() &&
                   m_handlers.back().funcIndex == funcIndex) {
                CatchFrame cf = m_handlers.back();
                m_handlers.pop_back();
                if (m_stack.size() > cf.stackBase) m_stack.resize(cf.stackBase);
                m_stack.push_back(ex.value); // the catch handler binds it
                ip = cf.catchIp;
                handled = true;
                break;
            }
            if (!handled) {
                // No handler in this frame: restore frames and propagate.
                if (m_locals.size() > localBase) m_locals.resize(localBase);
                if (m_stack.size() > stackBase) m_stack.resize(stackBase);
                m_handlers.resize(handlerBase);
                throw;
            }
            // Handler found: continue the for(;;) loop to re-enter while
            continue;
        }
        catch (...) {
            // Any other exception (e.g. runtime errors from division by zero):
            // restore this frame's stack/locals/handlers before propagating so
            // the VM is never left in a corrupt state for the next call.
            if (m_locals.size() > localBase) m_locals.resize(localBase);
            if (m_stack.size() > stackBase) m_stack.resize(stackBase);
            m_handlers.resize(handlerBase);
            throw;
        }
    } // end for(;;)

funcExit:
    if (m_locals.size() > localBase) m_locals.resize(localBase);
    if (m_stack.size() > stackBase) m_stack.resize(stackBase);
    m_handlers.resize(handlerBase);
    return result;
}
