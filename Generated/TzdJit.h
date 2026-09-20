#pragma once
// 1. ??????? (C++ Standard Library)
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <any>

#include "llvm/Support/Compiler.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/GlobalVariable.h"

#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"

#include "TzdLangBaseVisitor.h"
#include "TzdOop.h"

// Forward declaration to break circular include dependency
// (TzdOop.h includes TzdInterpreter.h which includes TzdJit.h which includes TzdOop.h)
class TzdClassDef;

struct TzdJitConfig {
    int optLevel = 2;                  // 0 = Off/Debug, 1 = Basic, 2 = Standard (Default), 3 = Aggressive
    int inlineThreshold = 250;         // Inlining threshold (0 = disable, 250 = default, 500 = aggressive)
    bool enableAstInlining = true;     // AST-level small function inlining
    bool enableMathIntrinsics = true;  // Direct LLVM math intrinsics
    bool enableLoopUnroll = true;      // Loop unrolling for O2/O3
    bool enableJitDebug = false;       // JIT debugging interface / safepoints
    int maxInlineDepth = 4;            // Max nested inlining depth
    int maxInlineStmts = 25;           // Max statements in inlined function
};

struct JittedFunctionInfo {
    std::string name;
    std::string internalName;
    std::string functionName;
    std::string internalSymbolName;
    void* entryAddress = nullptr;
    void* workerAddress = nullptr;
    void* nativeWorkerAddress = nullptr;
    void* nativeAddress = nullptr;
    int optLevel = 2;
    int paramCount = 0;
    size_t irInstructionCount = 0;
    std::string irDump;
    std::string llvmIR;
    bool inlined = false;
    bool isInlined = false;
};

/**
 * TzdJitEngine: LLVM ORC JIT Engine with Multi-Tier Optimization & Debug Interface
 */
class TzdJitEngine {
public:
    TzdJitEngine();
    ~TzdJitEngine();
    void registerRuntimeSymbols();
    void addModule(llvm::orc::ThreadSafeModule TSM);
    void jitModule(std::unique_ptr<llvm::Module> M);
    llvm::Expected<llvm::orc::ExecutorAddr>
        lookupSymbol(const std::string& name);
    void* lookupSymbolAsPtr(const std::string& name);
    llvm::LLVMContext& getContext();
    llvm::orc::ThreadSafeContext& getThreadSafeContext();

    // Register worker pointer for on-demand compiled functions (TCO support)
    void registerWorkerForSymbol(const std::string& internalName);

    // DataLayout / Triple
    const llvm::DataLayout& getDataLayout() const;
    std::string getTargetTriple() const;
    void executeFunction(const std::string& name, void* interp = nullptr, void* retVal = nullptr);

    // --- Optimization Level & Inlining Configuration ---
    static TzdJitConfig& getConfig();
    static void setOptLevel(int level);
    static int getOptLevel();
    static void setInlineThreshold(int threshold);
    static int getInlineThreshold();
    static void setJitDebugEnabled(bool enabled);
    static bool isJitDebugEnabled();
    static void setAstInliningEnabled(bool enabled);
    static bool isAstInliningEnabled();

    // --- JIT Debug & Inspection Registry ---
    static void registerJittedFunction(const JittedFunctionInfo& info);
    static std::vector<JittedFunctionInfo> getJittedFunctions();
    static JittedFunctionInfo* getJittedFunction(const std::string& name);
    static std::string dumpJitIR(const std::string& name);
    static size_t getJitCompiledCount();
    static size_t getTotalInlinedCalls();
    static void recordInlinedCall();

private:
    void initLLJIT();

private:
    std::unique_ptr<llvm::orc::LLJIT> m_lljit;
    llvm::orc::ThreadSafeContext m_tsc;
};

class TzdCompiler : public TzdLangBaseVisitor {
public:
    TzdCompiler(TzdJitEngine& jit, const std::string& moduleName);
    virtual ~TzdCompiler();

    llvm::Value* toNativeBool(llvm::Value* val);

    // Inline rt_store_native_to_ptr: direct GEP+Store on TzdValue fields
    void inlineStoreNativeToPtr(llvm::Value* dest, llvm::Value* nativeDouble);
    // Inline rt_to_double_fast: direct GEP+Load on TzdValue.dVal
    llvm::Value* inlineToDoubleFast(llvm::Value* src);

    // Advanced Inlining (AST-level small function inlining & math intrinsics)
    bool tryInlineFunction(const std::string& funcName, const std::vector<TzdLangParser::ExpressionContext*>& exprs, llvm::Value*& result, llvm::Value* receiverVal = nullptr, const std::string& explicitClassName = "");
    bool tryInlineMathIntrinsic(const std::string& funcName, const std::vector<TzdLangParser::ExpressionContext*>& exprs, llvm::Value*& result);

    std::unique_ptr<llvm::Module> getModule();
     std::unique_ptr<llvm::Module> extractModule();

     llvm::AllocaInst* CreateEntryBlockAlloca(const std::string& VarName) {
         llvm::Function* TheFunction = m_builder.GetInsertBlock()->getParent();
         llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
         return TmpB.CreateAlloca(m_ptrTy, nullptr, VarName);
     }

     llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Type* Ty, llvm::Value* ArraySize, const std::string& Name);

     void initLLVMTypes();

     llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Type* Ty, const std::string& Name, llvm::Value* ArraySize);

    void compileNamedFunction(TzdLangParser::BlockContext* block, TzdLangParser::ParamListContext* params, const std::string& internalName);
    void compileNamedFunction(TzdLangParser::BlockContext* block, const std::vector<std::string>& paramNames, const std::string& internalName);
    void compileClassMethod(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::MethodDeclContext* methodCtx);
    void compileClassMethod(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::MethodDeclContext* methodCtx, const std::string& internalName);
    void compileConstructor(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::ConstructorDeclContext* ctorCtx, const std::string& internalName);
    void compileNamedFunction(TzdLangParser::FunctionDeclarationContext* ctx, const std::string& internalName);

    void setupExternalFunctions();
    llvm::Function* getRtFunc(const std::string& name);
    llvm::orc::ThreadSafeModule extractThreadSafeModule();
    llvm::Value* boxToTzdValue(llvm::Value* val);

    virtual std::any visitProgram(TzdLangParser::ProgramContext* ctx) override;
    virtual std::any visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) override;
    virtual std::any visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) override;

    virtual std::any visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) override;
    virtual std::any visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext* ctx) override;
    virtual std::any visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext* ctx) override;
    virtual std::any visitExprStmt(TzdLangParser::ExprStmtContext* ctx) override;
    virtual std::any visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) override;
    virtual std::any visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) override;

    virtual std::any visitBlock(TzdLangParser::BlockContext* ctx) override;
    virtual std::any visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) override;
    virtual std::any visitMapLiteralExpr(TzdLangParser::MapLiteralExprContext* ctx) override;
    virtual std::any visitIndexExpr(TzdLangParser::IndexExprContext* ctx) override;

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
    virtual std::any visitIfStmt(TzdLangParser::IfStmtContext* ctx) override;
    virtual std::any visitForInit(TzdLangParser::ForInitContext* ctx) override;

    virtual std::any visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) override;
    virtual std::any visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) override;
    virtual std::any visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) override;
    virtual std::any visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) override;
    virtual std::any visitParenExpr(TzdLangParser::ParenExprContext* ctx) override;

    virtual std::any visitCastExpr(TzdLangParser::CastExprContext* ctx) override;
    virtual std::any visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) override;
    llvm::Value* ensureNativeDouble(llvm::Value* v);
    virtual std::any visitIdExpr(TzdLangParser::IdExprContext* ctx) override;
    virtual std::any visitIntExpr(TzdLangParser::IntExprContext* ctx) override;
    virtual std::any visitFloatExpr(TzdLangParser::FloatExprContext* ctx) override;
    virtual std::any visitStringExpr(TzdLangParser::StringExprContext* ctx) override;
    virtual std::any visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) override;
    virtual std::any visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) override;
    virtual std::any visitImportStmt(TzdLangParser::ImportStmtContext* ctx) override;
    virtual std::any visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) override;

    virtual std::any visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) override;
    virtual std::any visitNewExpr(TzdLangParser::NewExprContext* ctx) override;
    virtual std::any visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) override;
    virtual std::any visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) override;
    virtual std::any visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext* ctx) override;
    virtual std::any visitEnumDeclaration(TzdLangParser::EnumDeclarationContext* ctx) override;
    virtual std::any visitSuperExpr(TzdLangParser::SuperExprContext* ctx) override;
    virtual std::any visitCallExpr(TzdLangParser::CallExprContext* ctx) override;
    virtual std::any visitNullExpr(TzdLangParser::NullExprContext* ctx) override;
    virtual std::any visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) override;
    virtual std::any visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) override;

private:
    TzdJitEngine& m_jitEngine;
    llvm::orc::ThreadSafeContext m_tsc;
    llvm::LLVMContext& m_context;
    llvm::IRBuilder<> m_builder;
    std::unique_ptr<llvm::Module> m_module;

    std::unordered_map<std::string, llvm::Value*> m_namedValues;

    llvm::Type* m_ptrTy;
    llvm::Type* m_doubleTy;
    llvm::Type* m_boolTy;
    llvm::Type* m_voidTy;
    llvm::Value* m_currentRetPtr;
    llvm::GlobalVariable* m_globalRet;

    llvm::Type* m_int32Ty;
    llvm::StructType* m_tzdValueTy;

    const int TYPE_FIELD_INDEX = 1;
    const int DVAL_INDEX = 3;

    std::unordered_map<std::string, llvm::Value*> m_nativeDoubleLocals;
    std::unordered_set<std::string> m_declaredLocals;
    llvm::Value* castToNativeDouble(llvm::Value* val);
    llvm::Value* boxDouble(llvm::Value* nativeVal);

    struct LoopLabels {
        llvm::BasicBlock* continueBB = nullptr;
        llvm::BasicBlock* breakBB = nullptr;
    };
    std::vector<LoopLabels> m_loopStack;
    std::vector<llvm::BasicBlock*> m_switchEndStack;

    bool emitMemberIncDec(llvm::Value*& result, TzdLangParser::ExpressionContext* lhsCtx, bool isInc, bool isPrefix);

private:
    TzdClassDef* m_currentClassDef = nullptr;
    std::unordered_map<std::string, int> m_currentClassFieldMap;
    TzdSelector internSelectorConstant(const std::string& name);
    std::unordered_map<std::string, TzdSelector> m_selectorIds;
    std::unordered_map<std::string, std::string> m_varClassTypes;
};