#pragma once

#include <any>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <functional>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <thread>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <stddef.h>

#include <asmjit/asmjit.h>

#include "antlr4-runtime.h"
#include "TzdLangLexer.h"
#include "TzdLangParser.h"
#include "TzdLangBaseVisitor.h"
#include "../Plots/pbPlots.hpp"
#include "../Plots/supportLib.hpp"
#include "TzdNativeModule.h"
#include "TzdFFIAdapter.h"
#include "TzdOop.h"
#include "TzdJit.h"

#include <Eigen/Dense>
#include <matplot/matplot.h>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#include <mutex>
#pragma comment (lib, "Gdiplus.lib")
using namespace Gdiplus;
#endif

#if defined(__GNUC__) || defined(__clang__)
#define PREFETCH(addr) __builtin_prefetch(addr, 0, 1)
#elif defined(_MSC_VER)
#include <intrin.h>
#define PREFETCH(addr) _mm_prefetch((char*)(addr), _MM_HINT_T0)
#else
#define PREFETCH(addr) (void)0 
#endif

namespace fs = std::filesystem;

class TzdClassDef;
class TzdInstance;
struct ClassMethod; // forward-declared for callMethod(); fully defined in TzdOop.h

std::string Utf8ToAnsi(const std::string& str);

struct ScriptModule {
    std::string source;
    antlr4::ANTLRInputStream* input = nullptr;
    TzdLangLexer* lexer = nullptr;
    antlr4::CommonTokenStream* tokens = nullptr;
    TzdLangParser* parser = nullptr;
    TzdLangParser::ProgramContext* tree = nullptr;

    ~ScriptModule() {
        if (parser) delete parser;
        if (tokens) delete tokens;
        if (lexer) delete lexer;
        if (input) delete input;
    }
};


struct LastPlotState {
    bool active = false;
    std::vector<std::string> targetFuncNames;
    double start = 0.0;
    double end = 0.0;
    double step = 0.0;
};

struct TzdValue {
    enum Type {
        NONE,            // Null / void / uninitialized
        SBYTE, BYTE,     // 8-bit signed / unsigned integer (char / uint8)
        SHORT, USHORT,   // 16-bit signed / unsigned integer (int16 / uint16)
        INT, UINT,       // 32-bit signed / unsigned integer (int32 / uint32)
        LONG, ULONG,     // 64-bit signed / unsigned integer (int64 / uint64)
        FLOAT, DOUBLE,   // Floating-point number (float32 / float64)
        BOOL,            // Boolean flag (true / false)
        STRING,          // String literal or dynamic text
        POINTER,         // Raw memory pointer (void*)
        ARRAY,           // Array / sequential dynamic list
        MAP,             // Hash map / dictionary (associative container)
        FUNCTION,        // User-defined / bytecode function
        NATIVE_FUNCTION, // Native C/C++ host callback function
        CLASS_DEF,       // Class definition / prototype metadata
        INSTANCE,        // Class object instance
        ENUM_VAL,        // Enumeration variant / constant value
        ERROR_VAL,       // Error object or status (similar to Go error)
        FUTURE,          // Asynchronous Future / Promise handle
        ANY_REF,         // Generic type-erased reference (std::any wrapper)
        TENSOR           // PyTorch tensor (auto-managed VRAM lifecycle)
    };
    std::vector<std::string> annotations;
    Type type = NONE;
    std::string name = "";

    double dVal = 0.0;
    long long lVal = 0;
    unsigned long long ulVal = 0;
    void* ptrVal = nullptr;
    std::string sVal = "";
    bool bVal = false;

    std::vector<TzdValue> arrVal;
    std::unordered_map<std::string, TzdValue> mapVal;

    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    TzdLangParser::BlockContext* funcBody = nullptr;
    using NativeFuncType = std::function<TzdValue(const std::vector<TzdValue>&)>;
    NativeFuncType nativeFunc;

    TzdClassDef* classDefVal = nullptr;
    TzdInstance* instanceVal = nullptr;

    std::string sourceFile = "";
    int line = 0;
    int column = 0;

    TzdValue() : type(NONE), lVal(0), dVal(0.0), bVal(false), instanceVal(nullptr) {}
    TzdValue(TzdClassDef* c) : type(CLASS_DEF), classDefVal(c) {}
    TzdValue(TzdInstance* i);

    TzdValue(const TzdValue& o);
    TzdValue(TzdValue&& o) noexcept;
    TzdValue& operator=(const TzdValue& o);
    TzdValue& operator=(TzdValue&& o) noexcept;
    ~TzdValue();
    void setInstance(TzdInstance* i);

    TzdValue(double v) : type(DOUBLE), dVal(v) {}
    TzdValue(float v) : type(FLOAT), dVal((double)v) {}

    TzdValue(long long v) : type(LONG), lVal(v) {}
    TzdValue(unsigned long long v) : type(ULONG), ulVal(v) {}
    TzdValue(int v) : type(INT), lVal((long long)v) {}
    TzdValue(unsigned int v) : type(UINT), ulVal((unsigned long long)v) {}
    TzdValue(short v) : type(SHORT), lVal((long long)v) {}

    TzdValue(char v) : type(SBYTE), lVal((long long)v) {}
    TzdValue(unsigned char v) : type(BYTE), ulVal((unsigned long long)v) {}

    TzdValue(void* v) : type(POINTER), ptrVal(v) {}
    TzdValue(std::string v) : type(STRING), sVal(v) {}
    TzdValue(const char* v) : type(STRING), sVal(v ? v : "") {}
    TzdValue(bool v) : type(BOOL), bVal(v) {}

    TzdValue(const std::vector<TzdValue>& v) : type(ARRAY), arrVal(v) {}
    TzdValue(const std::unordered_map<std::string, TzdValue>& v) : type(MAP), mapVal(v) {}

    TzdValue(std::string n, std::vector<std::string> p, TzdLangParser::BlockContext* ctx)
        : type(FUNCTION), name(n), params(p), funcBody(ctx) {
    }

    TzdValue(NativeFuncType func, std::string n = "")
        : type(NATIVE_FUNCTION), nativeFunc(func), name(n) {
    }

    static TzdValue Error(std::string msg) {
        TzdValue ev;
        ev.type = ERROR_VAL;
        ev.sVal = msg;
        return ev;
    }
    std::string jitInternalName;
    std::vector<double> nativeArr;  // Backing storage for unboxed double array (performance fast-path)
    bool isNativeDoubleArr = false; // Flag indicating if current array is a specialized native double array
    void (*jittedPtr)(void*, void*) = nullptr;
};

// Release the remaining instance references in the pool slot (refer to TzdInterpreter.cpp for details).
// Use out-of-line helpers to avoid inlining of next() in this case where the complete type of TzdInstance is required -
// TzdOop.cpp is compiled with circular includes and TzdInstance is not yet defined.
void tzdPoolSlotReleaseInstance(TzdValue* v);

// Stack overflow detection (defined in TzdInterpreter.cpp) is shared by the interpreter and the JIT call bridge.
bool tzdStackNearOverflow();

struct JitValuePool {
    static const size_t POOL_SIZE = 4096;
    std::vector<TzdValue> storage;
    size_t cursor = 0;

    inline TzdValue* next() {
        if (storage.empty()) storage.resize(POOL_SIZE);
        size_t next_cursor = (cursor + 1) & (POOL_SIZE - 1);
        TzdValue* v = &storage[cursor];
        cursor = next_cursor;
        // Before reusing the slot, release the reference of the previous instance - the interception point for JIT hot path leaks
        tzdPoolSlotReleaseInstance(v);
        [[unlikely]] if (v->type >= TzdValue::STRING) {
            switch (v->type) {
            case TzdValue::STRING: v->sVal.clear(); break;
            case TzdValue::ARRAY:  v->arrVal.clear(); break;
            case TzdValue::MAP:    v->mapVal.clear(); break;
            default:
                break;
            }
        }
        v->type = TzdValue::NONE;
        return v;
    }

    void reset() { cursor = 0; }

    ~JitValuePool() {
        for (auto& item : storage) item.instanceVal = nullptr;
    }
};

extern thread_local JitValuePool g_JitPool;

class TzdRuntimeException : public std::runtime_error {
public:
    size_t line;
    size_t column;
    std::vector<std::string> stackTrace;
    TzdRuntimeException(const std::string& msg, antlr4::Token* token, std::vector<std::string> trace = {})
        : std::runtime_error(msg),
        line(token ? token->getLine() : 0),
        column(token ? token->getCharPositionInLine() : 0),
        stackTrace(std::move(trace)) {}
};

class TzdReturnException : public std::exception {
public:
    TzdValue value;
    TzdReturnException(TzdValue v) : value(v) {}
};

class TzdBreakException : public std::exception {};
class TzdContinueException : public std::exception {};

class TzdThrowException : public std::exception {
public:
    TzdValue value;
    std::vector<std::string> stackTrace;
    explicit TzdThrowException(TzdValue v, std::vector<std::string> trace = {})
        : value(std::move(v)), stackTrace(std::move(trace)) {}
};
class TzdErrorListener : public antlr4::BaseErrorListener {
public:
    virtual void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol,
        size_t line, size_t charPositionInLine,
        const std::string& msg, std::exception_ptr e) override;
};

class TzdErrorHandler {
public:
    static void report(const std::string& type, size_t line, size_t column,
        const std::string& msg, const std::string& sourceCode,
        const std::vector<std::string>& stackTrace = {});
};

class TzdJitEngine;
class TzdCompiler;

// Forward declarations for bytecode VM (defined in TzdBytecode.h)
struct BytecodeModule;
class TzdBytecodeVM;

// Selector cache for a single AST member-access site. Keyed by the AST node
// pointer (stable for the lifetime of the loaded module). Only the selector
// and the member name are cached — never an unguarded receiver/method
// pointer, since the receiver type at a site may change between calls.
struct MemberAccessSiteCache {
    TzdSelector selector = 0;
    std::string name;
    bool resolved = false;
};

struct TzdCallFrame {
    TzdValue* thisPtr = nullptr;
    TzdValue* args = nullptr;
    int argCount = 0;
};

class TzdInterpreter : public TzdLangBaseVisitor {
public:
    friend class TzdCompiler;
    LastPlotState m_lastPlot;
    std::unique_ptr<TzdJitEngine> m_jitEngine;
    std::unique_ptr<TzdCompiler> m_compiler;
    std::set<std::string> m_pendingJitFunctions;
    using FunctionRedefinedCallback = std::function<void(const std::string&, const TzdValue&, antlr4::ParserRuleContext*)>;
    std::vector<std::unordered_map<std::string, TzdValue>> scopes;
    bool m_isCompiling = false;
    std::vector<ScriptModule*> loadedModules;
    std::string m_currentSource;
    std::set<std::string> m_importedFiles;

    void emitFunctionRedefined(const std::string& name, const TzdValue& newVal, antlr4::ParserRuleContext* ctx) {
        for (auto& callback : redefinitionListeners) {
            callback(name, newVal, ctx);
        }
    }

    std::vector<std::string> m_includePaths;
    std::vector<fs::path> m_scriptPathStack;
   
    std::string resolveImportPath(const std::string& inputPath);
    TzdInterpreter();
    ~TzdInterpreter(); // defined in .cpp (needs BytecodeModule complete type)

    void compileScriptToMemory(const std::string& code);

    void compileFunctionToMemory(const std::string& funcName);

    TzdValue getVariable(const std::string& name, antlr4::ParserRuleContext* ctx);
    void setVariable(const std::string& name, TzdValue val);

    void loadScript(std::string code);
    void loadScriptFromFile(const std::string& filePath);

    // Compile script to bytecode file (implementation in .cpp uses TzdBytecode.h)
    bool compileToBytecodeFile(const std::string& code, const std::string& outPath);
    // Load and execute bytecode file
    bool executeBytecodeFile(const std::string& bcPath);

    void addIncludePath(const std::string& path) {
        m_includePaths.push_back(path);
    }
    
    void tryJitCompile(TzdValue& funcVal);

    void initNativeFunctions();
    void registerNativeFunction(const std::string& name, TzdValue::NativeFuncType func) {
        if (!scopes.empty()) {
            scopes[0][name] = TzdValue(func);
        }
    }

    virtual std::any visit(antlr4::tree::ParseTree *tree) override;
    virtual std::any visitProgram(TzdLangParser::ProgramContext* ctx) override;

    void prepareForScript();
    void jitAllFunctions();
    void jitPendingModule();

    virtual std::any visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) override;
    virtual std::any visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) override;

    virtual std::any visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) override;
    virtual std::any visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext* ctx) override;
    virtual std::any visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext* ctx) override;
    virtual std::any visitExprStmt(TzdLangParser::ExprStmtContext* ctx) override;
    virtual std::any visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) override;
    virtual std::any visitBlock(TzdLangParser::BlockContext* ctx) override;
    virtual std::any visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) override;
    virtual std::any visitIndexExpr(TzdLangParser::IndexExprContext* ctx) override;
    virtual std::any visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) override;

    void internalRenderPlot(const std::vector<TzdValue>& functions, double start, double end, double step);

    void setGlobalVariable(const std::string& name, const TzdValue& val);

    void mapJitSymbolsToValue();

    TzdValue callFunction(const TzdValue& func, const std::vector<TzdValue>& args);

    // Direct method call that avoids building (and copying) a bound TzdValue.
    // `method` is the resolved ClassMethod entry; `receiver` is the actual
    // instance (nullptr for static methods). Preserves arg/type checks, stack
    // traces, debugger fallback and JIT behaviour. Native methods invoke
    // their native wrapper; script methods go through JIT or tree-walk.
    TzdValue callMethod(ClassMethod& method, TzdInstance* receiver,
        const std::vector<TzdValue>& args);

    TzdValue executeFunction(const TzdValue& funcVal, const std::vector<TzdValue>& args);

    virtual std::any visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) override;
    virtual std::any visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) override;
    virtual std::any visitPowerExpr(TzdLangParser::PowerExprContext* ctx) override;
    virtual std::any visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) override;

    virtual std::any visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) override;
    virtual std::any visitForStmt(TzdLangParser::ForStmtContext* ctx) override;

    virtual std::any visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) override;

    virtual std::any visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) override;
    virtual std::any visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) override;
    virtual std::any visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) override;
    virtual std::any visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) override;
    virtual std::any visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) override;
    virtual std::any visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) override;

    virtual std::any visitIfStmt(TzdLangParser::IfStmtContext* ctx) override;
    virtual std::any visitForInit(TzdLangParser::ForInitContext* ctx) override;

    virtual std::any visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) override;
    virtual std::any visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) override;
    virtual std::any visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) override;
    virtual std::any visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) override;
    virtual std::any visitParenExpr(TzdLangParser::ParenExprContext* ctx) override;

    virtual std::any visitCastExpr(TzdLangParser::CastExprContext* ctx) override;

    virtual std::any visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) override;
    virtual std::any visitIdExpr(TzdLangParser::IdExprContext* ctx) override;
    virtual std::any visitIntExpr(TzdLangParser::IntExprContext* ctx) override;
    virtual std::any visitFloatExpr(TzdLangParser::FloatExprContext* ctx) override;
    virtual std::any visitStringExpr(TzdLangParser::StringExprContext* ctx) override;
    virtual std::any visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) override;
    virtual std::any visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) override;
    virtual std::any visitImportStmt(TzdLangParser::ImportStmtContext* ctx) override;
    virtual std::any visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) override;

    virtual std::any visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) override;
    void validateAnnotationUsage(TzdLangParser::AnnotationUsageContext* ctx);
    bool isTypeValid(const std::string& typeName);
    bool checkParamValueType(const std::string& typeName, const TzdValue& val);
    virtual std::any visitNewExpr(TzdLangParser::NewExprContext* ctx) override;
    virtual std::any visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) override;
    virtual std::any visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) override;
    void checkSymbolCollision(const std::string& name, antlr4::ParserRuleContext* ctx);
    virtual std::any visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext* ctx) override;
    virtual std::any visitEnumDeclaration(TzdLangParser::EnumDeclarationContext* ctx) override;
    virtual std::any visitSuperExpr(TzdLangParser::SuperExprContext* ctx) override;
    virtual std::any visitCallExpr(TzdLangParser::CallExprContext* ctx) override;

    virtual std::any visitNullExpr(TzdLangParser::NullExprContext* ctx) override;
    std::vector<FunctionRedefinedCallback> redefinitionListeners;

    void onFunctionRedefined(FunctionRedefinedCallback cb) {
        redefinitionListeners.push_back(cb);
    }
    static double getAsDouble(std::any value);
    static double getAsDoubleInternal(const TzdValue& v);
    static bool isTruthy(const TzdValue& v);
    static bool valuesEqual(const TzdValue& l, const TzdValue& r);
    static std::string getAsString(std::any value);

    void compileCurrentContext();

    void* trackJitValue(TzdValue* ptr) {
        m_jitGarbage.push_back(ptr);
        return ptr;
    }

    void clearJitMemory() {
        for (auto p : m_jitGarbage) delete p;
        m_jitGarbage.clear();
    }

    void clearJitError() {
        m_hasJitError = false;
        m_lastJitError.clear();
    }

    void reportJitError(const std::string& msg) {
        m_hasJitError = true;
        m_lastJitError = msg;
        m_jitErrorTrace = m_callStackFrames;
    }

    std::string m_lastJitError;
    bool m_hasJitError = false;
    size_t m_jitLine = 0;
    size_t m_jitColumn = 0;
    std::vector<std::string> m_jitErrorTrace;
    std::unique_ptr<TzdThrowException> m_jitUnhandledThrow;

    std::vector<TzdValue*> m_jitGarbage;
    std::vector<TzdValue> m_currentArgs;
    std::unordered_map<std::string, std::string> m_jitNameToUserMap;
    uint64_t m_funcVersion = 0;

    std::vector<std::vector<TzdValue>> m_argFrameStack;
    std::vector<TzdValue*> m_argPtrStack;
    std::vector<TzdCallFrame> m_callFrameStack;

    size_t m_callDepth = 0;
    size_t m_maxCallDepth = 4096;
    bool m_hadRuntimeError = false;
    std::vector<std::string> m_callStackFrames;
    std::vector<std::string> m_debugFileStack;
    bool m_silentMode = false;
    bool m_noJit = false; // When true, disable all JIT compilation (interpreter-only mode)

    // Bytecode VM integration: when set, script functions are executed
    // via the bytecode VM instead of tree-walking interpretation.
    std::unique_ptr<BytecodeModule> m_bytecodeModule;
    std::unique_ptr<TzdBytecodeVM> m_bytecodeVM;
    bool m_useBytecodeVM = false;

    // When true (--interpreter / --tree-walk), force pure tree-walk: disable
    // JIT (m_noJit) and prevent the bytecode VM from engaging. loadScript()
    // must not re-enable the bytecode VM while this flag is set.
    bool m_forceInterpreter = false;

    // Per-AST-site selector cache. Keyed by the MemberAccessExpr AST node
    // pointer (stable for the lifetime of the loaded module).
    std::unordered_map<const antlr4::ParserRuleContext*, MemberAccessSiteCache> m_memberSelectorCache;

private:
    // Common core shared by callFunction() (script branch) and callMethod()
    // (script branch). Takes the actual receiver separately rather than
    // embedding it in a bound TzdValue, and reads function metadata by
    // reference — no template TzdValue copy. Preserves arg/type checks,
    // stack traces, debugger fallback (skip JIT when g_DebugActive), and JIT
    // behaviour. Scope unwinding is exception-safe for every path.
    TzdValue callScriptFunction(const std::string& name,
        const std::vector<std::string>& params,
        const std::vector<std::string>& paramTypes,
        TzdLangParser::BlockContext* funcBody,
        void (*jittedPtr)(void*, void*),
        TzdInstance* receiver,
        const std::vector<TzdValue>& args,
        const std::string& sourceFile,
        int line);

    // Resolve (and cache) the TzdSelector + member name for a member-access
    // AST site. The cache is keyed by the AST node pointer and is stable for
    // the lifetime of the loaded module; the selector/name are immutable once
    // interned.
    MemberAccessSiteCache& resolveMemberAccessSite(
        antlr4::ParserRuleContext* ctx, const std::string& memberName);
};

extern TzdInterpreter* g_CurrentInterpreter;

// ============================================================================
// Tensor lifecycle management hooks (implemented in TzdPyTorch.cpp)
// These are weak/no-op when PyTorch is not linked; TzdValue copy control
// calls them for TENSOR-type values to guarantee zero VRAM leaks.
// ============================================================================
extern "C" void tzdTensorRetain(void* ptr);
extern "C" void tzdTensorRelease(void* ptr);
