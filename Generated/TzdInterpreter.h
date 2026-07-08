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
        NONE,           // null / void
        SBYTE, BYTE,    // 8?? ??????/????? (char/byte)
        SHORT, USHORT,  // 16??
        INT, UINT,      // 32??
        LONG, ULONG,    // 64?? (int64/uint64)
        FLOAT, DOUBLE,  // ???? (float32/64)
        BOOL,           // ????
        STRING,         // ?????
        POINTER,        // ????? (void*)
        ARRAY,          // ???? / ????
        MAP,            // ??? / ???????? (????)
        FUNCTION,       // ??????????
        NATIVE_FUNCTION,// C++ ??????
        CLASS_DEF,      // ???? (?????)
        INSTANCE,       // ???????
        ENUM_VAL,       // ????
        ERROR_VAL,      // ???????? (???? Go ?? error)
        FUTURE,         // ????? / Promise (????)
        ANY_REF         // ??????????? (std::any)
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
    std::unordered_map<std::string, TzdValue> mapVal; // ??? var m = { "key": 1 };

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
    TzdValue(TzdInstance* i) : type(INSTANCE), instanceVal(i) {}

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
    std::vector<double> nativeArr;  // ?? arrVal ????
    bool isNativeDoubleArr = false; // ????????? double ????
    void (*jittedPtr)(void*, void*) = nullptr;
};

struct JitValuePool {
    static const size_t POOL_SIZE = 262144;
    TzdValue storage[POOL_SIZE];
    size_t cursor = 0;

    inline TzdValue* next() {
        size_t next_cursor = (cursor + 1) & (POOL_SIZE - 1);
        TzdValue* v = &storage[cursor];
        cursor = next_cursor;
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
};

static thread_local JitValuePool g_JitPool;

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

class TzdJitCompiler;

class TzdInterpreter : public TzdLangBaseVisitor {
public:
    friend class TzdJitCompiler;
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
    TzdInterpreter() {
        m_jitEngine = std::make_unique<TzdJitEngine>();
        m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "main_module");
        scopes.push_back({});
    }

    ~TzdInterpreter() {
        for (auto mod : loadedModules) delete mod;
        loadedModules.clear();
    }

    void compileScriptToMemory(const std::string& code);

    void compileFunctionToMemory(const std::string& funcName);

    TzdValue getVariable(const std::string& name, antlr4::ParserRuleContext* ctx);
    void setVariable(const std::string& name, TzdValue val);

    void loadScript(std::string code);
    void loadScriptFromFile(const std::string& filePath);

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

    /**
    * ????????????????????????????????
    */
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

    size_t m_callDepth = 0;
    size_t m_maxCallDepth = 4096;
    bool m_hadRuntimeError = false;
    std::vector<std::string> m_callStackFrames;
    std::vector<std::string> m_debugFileStack;
    bool m_silentMode = false;
};

extern TzdInterpreter* g_CurrentInterpreter;
