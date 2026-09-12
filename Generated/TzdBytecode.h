// ============================================================================
// TzdBytecode.h - Bytecode VM instruction set, compiler, and VM declarations
// (Implementation to be added)
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <functional>
#include "TzdInterpreter.h"
#include "TzdLangBaseVisitor.h"

// Bytecode opcode enumeration
enum class OpCode : uint8_t {
    // Constants
    PUSH_DOUBLE = 0x01,
    PUSH_STRING = 0x02,
    PUSH_BOOL   = 0x03,
    PUSH_NULL   = 0x04,
    PUSH_INT    = 0x05,

    // Variables
    LOAD_LOCAL  = 0x10,
    STORE_LOCAL = 0x11,
    LOAD_VAR    = 0x12,
    STORE_VAR   = 0x13,

    // Arithmetic
    ADD = 0x20, SUB = 0x21, MUL = 0x22, DIV = 0x23,
    MOD = 0x24, POW = 0x25, NEG = 0x26,

    // Comparison
    EQ = 0x30, NE = 0x31, LT = 0x32, LE = 0x33,
    GT = 0x34, GE = 0x35,

    // Logical
    AND = 0x40, OR = 0x41, NOT = 0x42,

    // Control flow
    JMP        = 0x50,
    JMP_FALSE  = 0x51,
    JMP_TRUE   = 0x52,

    // Function calls
    CALL_FUNC  = 0x60,
    CALL_NATIVE= 0x61,
    CALL_METHOD= 0x62,
    RET        = 0x63,
    RET_VOID   = 0x64,

    // Stack ops
    POP = 0x70, DUP = 0x71,

    // Other
    PRINT = 0x80,
    THROW = 0x81,
    SCOPE_PUSH = 0x82,
    SCOPE_POP  = 0x83,

    // OOP / container operations (added for full VM support)
    LOAD_MEMBER  = 0x84,   // arg1 = name const idx; pop obj, push obj.<name>
    STORE_MEMBER = 0x85,   // arg1 = name const idx; pop value, pop obj, obj.<name> = value
    LOAD_INDEX   = 0x86,   // pop index, pop container, push container[index]
    STORE_INDEX  = 0x87,   // pop value, pop index, pop container, container[index] = value
    NEW_OBJECT   = 0x88,   // arg1 = class name const idx, arg2 = arg count; pop args, push instance
    MAKE_ARRAY   = 0x89,   // arg1 = element count; pop count elements, push array
    CALL_VALUE   = 0x8A,   // arg1 = arg count; pop args, pop callee value, call it
    TRY          = 0x8B,   // arg1 = catch handler offset; register catch target
    TRY_END      = 0x8C,   // unregister current catch target
    HALT = 0xFF,
};

// Bytecode instruction
struct Instruction {
    OpCode op;
    int32_t arg1;  // Generic operand (index, offset, etc.)
    int32_t arg2;  // Second operand (for some instructions)
    mutable int32_t cache = -1;  // Runtime cache: CALL_FUNC caches funcIndex
    mutable void* cacheClass = nullptr;
    mutable int32_t cacheIndex = -1;
};

// Constant pool entry
struct ConstEntry {
    enum Type { DOUBLE, STRING, INT, BOOL, BIGINT } type;
    double dVal;
    std::string sVal;
    int64_t iVal;
    bool bVal;
};

// Bytecode function
// Lightweight static type tags for local variable type inference
enum class LocalType : uint8_t { Unknown=0, Int64, Float64, ObjRef };

struct BytecodeFunc {
    std::string name;
    int paramCount;
    int localCount;
    int32_t maxStackDepth = 0;  // computed by verifier; 0 = unknown
    std::vector<Instruction> code;
    std::vector<std::string> paramNames; // for closure scope storage
    std::vector<LocalType> localTypes;   // inferred type per local slot
};

// Bytecode module
struct BytecodeModule {
    std::string name;
    std::vector<ConstEntry> constants;
    std::vector<BytecodeFunc> functions;
    std::unordered_map<std::string, size_t> funcIndex;
    std::string sourceCode; // original source for class/enum/annotation declarations

    // .tzdc file format magic and version
    static constexpr uint32_t MAGIC = 0x43445A54; // "TZDC" in little-endian
    static constexpr uint32_t VERSION = 2; // bumped for paramNames + sourceCode
};

// Bytecode compiler: AST -> Bytecode
class TzdBytecodeCompiler : public TzdLangBaseVisitor {
public:
    TzdBytecodeCompiler();

    // Compile a script AST to bytecode
    BytecodeModule compile(TzdLangParser::ProgramContext* tree, const std::string& sourceCode = "");

    // Bytecode verifier: validates jump targets, constant indices, stack underflow,
    // and computes maxStackDepth for each function. Returns false on error.
    bool verifyModule(BytecodeModule& mod, std::string* errMsg = nullptr);

    // Save bytecode to .tzdc file
    bool saveToFile(const BytecodeModule& module, const std::string& path);

    // Load bytecode from .tzdc file
    BytecodeModule loadFromFile(const std::string& path);

    // ---- Statement visitors ----
    virtual std::any visitProgram(TzdLangParser::ProgramContext* ctx) override;
    virtual std::any visitBlockStmt(TzdLangParser::BlockStmtContext* ctx) override;
    virtual std::any visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) override;
    virtual std::any visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) override;
    virtual std::any visitVariableDeclaration(TzdLangParser::VariableDeclarationContext* ctx) override;
    virtual std::any visitExprStmt(TzdLangParser::ExprStmtContext* ctx) override;
    virtual std::any visitIfStmt(TzdLangParser::IfStmtContext* ctx) override;
    virtual std::any visitForStmt(TzdLangParser::ForStmtContext* ctx) override;
    virtual std::any visitForInit(TzdLangParser::ForInitContext* ctx) override;
    virtual std::any visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) override;
    virtual std::any visitBlock(TzdLangParser::BlockContext* ctx) override;
    virtual std::any visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) override;
    virtual std::any visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) override;
    virtual std::any visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) override;
    virtual std::any visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) override;
    virtual std::any visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) override;
    virtual std::any visitEmptyStmt(TzdLangParser::EmptyStmtContext* ctx) override;
    virtual std::any visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) override;

    // ---- Expression visitors ----
    virtual std::any visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) override;
    virtual std::any visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) override;
    virtual std::any visitPowerExpr(TzdLangParser::PowerExprContext* ctx) override;
    virtual std::any visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) override;
    virtual std::any visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) override;
    virtual std::any visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) override;
    virtual std::any visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) override;
    virtual std::any visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) override;
    virtual std::any visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) override;
    virtual std::any visitCallExpr(TzdLangParser::CallExprContext* ctx) override;
    virtual std::any visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) override;
    virtual std::any visitIndexExpr(TzdLangParser::IndexExprContext* ctx) override;
    virtual std::any visitNewExpr(TzdLangParser::NewExprContext* ctx) override;
    virtual std::any visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) override;
    virtual std::any visitParenExpr(TzdLangParser::ParenExprContext* ctx) override;
    virtual std::any visitIntExpr(TzdLangParser::IntExprContext* ctx) override;
    virtual std::any visitFloatExpr(TzdLangParser::FloatExprContext* ctx) override;
    virtual std::any visitStringExpr(TzdLangParser::StringExprContext* ctx) override;
    virtual std::any visitIdExpr(TzdLangParser::IdExprContext* ctx) override;
    virtual std::any visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) override;
    virtual std::any visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) override;
    virtual std::any visitNullExpr(TzdLangParser::NullExprContext* ctx) override;
    virtual std::any visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) override;
    virtual std::any visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) override;
    virtual std::any visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) override;
    virtual std::any visitAtomExpr(TzdLangParser::AtomExprContext* ctx) override;
    virtual std::any visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) override;
    virtual std::any visitSuperExpr(TzdLangParser::SuperExprContext* ctx) override;
    virtual std::any visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) override;
    virtual std::any visitCastExpr(TzdLangParser::CastExprContext* ctx) override;

private:
    BytecodeModule m_module;
    int m_localIndex;
    BytecodeFunc* m_currentFunc = nullptr;
    size_t m_currentFuncIdx = (size_t)-1; // stable index (survives vector realloc)

    // Lexical scope tracking: each scope maps variable name -> local slot index.
    std::vector<std::unordered_map<std::string, int>> m_scopeStack;
    // Names of functions compiled into the module (for call resolution).
    std::unordered_map<std::string, size_t> m_moduleFuncNames;

    // Break/continue patching for the innermost loop.
    struct LoopContext {
        int continueTarget;                 // known target for continue (-1 = forward ref)
        std::vector<size_t> breakJumps;     // instruction indices of JMP to patch to loop end
        std::vector<size_t> continueJumps;  // instruction indices of JMP to patch to continue target
    };
    std::vector<LoopContext> m_loops;

    int addConstant(double v);
    int addConstant(const std::string& v);
    int addConstant(int64_t v);
    int addConstant(bool v);
    int addConstant(const ConstEntry& e);

    void emit(OpCode op, int32_t arg1 = 0, int32_t arg2 = 0);
    size_t here() const;                    // index of next instruction to be emitted
    size_t emitJump(OpCode op);             // emit a jump with placeholder, return its index
    void patchJump(size_t instrIdx, int32_t target); // set a jump's target
    int resolveLocal(const std::string& name) const; // slot or -1
    int declareLocal(const std::string& name);       // allocate a new local slot
    void setLocalType(int slot, LocalType type);     // set inferred type for a local slot
    void pushScope();
    void popScope();
    void beginFunction(const std::string& name, int paramCount);
    void endFunction(bool emitReturn);
    bool isModuleFunction(const std::string& name) const;
    std::string getQualifiedName(TzdLangParser::QualifiedNameContext* ctx) const;
    std::vector<std::string> extractParamNames(TzdLangParser::ParamListContext* pl) const;
};

// Bytecode VM: execute bytecode
class TzdBytecodeVM {
public:
    TzdBytecodeVM(TzdInterpreter* interp);

    // Execute a bytecode module
    TzdValue execute(const BytecodeModule& module);

    // Execute a single function
    TzdValue callFunction(const BytecodeModule& module, const std::string& funcName,
                          const std::vector<TzdValue>& args);

    void disassemble(const BytecodeModule& module) const; // debug dump to stderr

private:
    TzdInterpreter* m_interp;
    const BytecodeModule* m_module = nullptr;
    std::vector<TzdValue> m_stack;
    std::vector<TzdValue> m_locals;

    // Active try/catch handlers (for THROW unwinding within bytecode).
    struct CatchFrame {
        size_t funcIndex;
        size_t catchIp;
        size_t stackBase;
        size_t localBase;
        std::string catchVar;
    };
    std::vector<CatchFrame> m_handlers;

    TzdValue runBytecodeFunc(const BytecodeModule& module, size_t funcIndex,
                             const std::vector<TzdValue>& args);
    TzdValue runBytecodeFunc(const BytecodeModule& module, size_t funcIndex,
                             const TzdValue* argsData, size_t argCount);
    TzdValue callValue(const BytecodeModule& module, const TzdValue& callee,
                       const std::vector<TzdValue>& args);
    TzdValue getMember(const TzdValue& obj, const std::string& name) const;
    void setMember(const TzdValue& obj, const std::string& name, const TzdValue& val) const;
};
