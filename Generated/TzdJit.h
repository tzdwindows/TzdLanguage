#pragma once
// 1. ??????? (C++ Standard Library)
#include <map>
#include <unordered_map>
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

/**
 * TzdJitEngine: ???? LLVM ?????????????????????????
 */
class TzdJitEngine {
public:
    // ????/????
    TzdJitEngine();
    ~TzdJitEngine();

    // ??????????????? rt_create_num ???
    void registerRuntimeSymbols();

    // ???????? ThreadSafeModule ????? JIT??????????
    void addModule(llvm::orc::ThreadSafeModule TSM);

    // ?????????????????? raw Module??????? ThreadSafeModule ?? move??
    void jitModule(std::unique_ptr<llvm::Module> M);

    // ?? symbol?????? llvm::Expected???????????????????
    llvm::Expected<llvm::orc::ExecutorAddr>
        lookupSymbol(const std::string& name);

    // ??? wrapper????????????????????????? nullptr??
    void* lookupSymbolAsPtr(const std::string& name);

    // ??????????? LLVMContext ??????? ThreadSafeContext ?????
    llvm::LLVMContext& getContext();

    // ?? Compiler ???????? ThreadSafeContext ????
    llvm::orc::ThreadSafeContext& getThreadSafeContext();

    // DataLayout / Triple
    const llvm::DataLayout& getDataLayout() const;
    std::string getTargetTriple() const;

    // ????????????? API ??????
    // ?????????????????????? (?????? void(*)(void*,void*) ?????)
    void executeFunction(const std::string& name, void* interp = nullptr, void* retVal = nullptr);

private:
    // ????? LLJIT??ThreadSafeContext
    void initLLJIT();

private:
    // ???????? LLJIT ???
    std::unique_ptr<llvm::orc::LLJIT> m_lljit;

    // ??????????? ThreadSafeContext???????????????????
    llvm::orc::ThreadSafeContext m_tsc;
};

/**
 * TzdCompiler: ??????? AST ?????? LLVM IR
 */
class TzdCompiler : public TzdLangBaseVisitor {
public:
    TzdCompiler(TzdJitEngine& jit, const std::string& moduleName);
    
    llvm::Value* toNativeBool(llvm::Value* val);

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

    void compileClassMethod(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::MethodDeclContext* methodCtx);
    void compileNamedFunction(TzdLangParser::BlockContext* block, TzdLangParser::ParamListContext* params, const std::string& internalName);
    void compileClassMethod(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::MethodDeclContext* methodCtx, const std::string& internalName);
    void compileConstructor(TzdLangParser::ClassDeclarationContext* classCtx, TzdLangParser::ConstructorDeclContext* ctorCtx, const std::string& internalName);
    void compileNamedFunction(TzdLangParser::FunctionDeclarationContext* ctx, const std::string& internalName);

    // ??????????????? (rt_...) ?????
    void setupExternalFunctions();
    llvm::Function* getRtFunc(const std::string& name);
    llvm::orc::ThreadSafeModule extractThreadSafeModule();
    llvm::Value* boxToTzdValue(llvm::Value* val);
    // --- ???? ANTLR Visitor ???? ---

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
    llvm::LLVMContext& m_context;
    TzdJitEngine& m_jitEngine;
    llvm::IRBuilder<> m_builder;
    std::unique_ptr<llvm::Module> m_module;
    llvm::orc::ThreadSafeContext m_tsc;

    std::unordered_map<std::string, llvm::Value*> m_namedValues;

    llvm::Type* m_ptrTy;
    llvm::Type* m_doubleTy;
    llvm::Type* m_boolTy;
    llvm::Type* m_voidTy;
    llvm::Value* m_currentRetPtr;
    llvm::GlobalVariable* m_globalRet;

    llvm::Type* m_int32Ty;
    llvm::StructType* m_tzdValueTy;

    // ???????????????????? C++ ??????????????????
    const int TYPE_FIELD_INDEX = 1; // annotations ?? 0, type ?? 1
    const int DVAL_INDEX = 3;       // name ?? 2, dVal ?? 3

    std::unordered_map<std::string, llvm::AllocaInst*> m_nativeDoubleLocals;
    llvm::Value* castToNativeDouble(llvm::Value* val);
    llvm::Value* boxDouble(llvm::Value* nativeVal);

    struct LoopLabels {
        llvm::BasicBlock* continueBB = nullptr;
        llvm::BasicBlock* breakBB = nullptr;
    };
    std::vector<LoopLabels> m_loopStack;
    std::vector<llvm::BasicBlock*> m_switchEndStack;

    bool emitMemberIncDec(llvm::Value*& result, TzdLangParser::ExpressionContext* lhsCtx, bool isInc, bool isPrefix);
};