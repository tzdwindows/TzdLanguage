// ============================================================================
// TzdNativeCodegen.h
// AST-to-Native C++ Code Generator for TzdLang AOT Machine Code Compiler
// Translates TzdLang AST directly into clean, high-performance C++20 code
// ============================================================================

#ifndef TZD_NATIVE_CODEGEN_H
#define TZD_NATIVE_CODEGEN_H

#include "Generated/TzdLangBaseVisitor.h"
#include "Generated/TzdLangParser.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>

namespace tzd {

class TzdNativeCodegen : public TzdLangBaseVisitor {
public:
    TzdNativeCodegen();
    virtual ~TzdNativeCodegen() = default;

    // Compile an ANTLR4 AST into complete standalone C++20 source code
    std::string generate(TzdLangParser::ProgramContext* tree, const std::string& scriptName, bool cpuOnly = false);

    // ---- Statement visitors ----
    virtual std::any visitProgram(TzdLangParser::ProgramContext* ctx) override;
    virtual std::any visitBlock(TzdLangParser::BlockContext* ctx) override;
    virtual std::any visitBlockStmt(TzdLangParser::BlockStmtContext* ctx) override;
    virtual std::any visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) override;
    virtual std::any visitVariableDeclaration(TzdLangParser::VariableDeclarationContext* ctx) override;
    virtual std::any visitIfStmt(TzdLangParser::IfStmtContext* ctx) override;
    virtual std::any visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) override;
    virtual std::any visitForStmt(TzdLangParser::ForStmtContext* ctx) override;
    virtual std::any visitForInit(TzdLangParser::ForInitContext* ctx) override;
    virtual std::any visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) override;
    virtual std::any visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) override;
    virtual std::any visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) override;
    virtual std::any visitExprStmt(TzdLangParser::ExprStmtContext* ctx) override;
    virtual std::any visitEmptyStmt(TzdLangParser::EmptyStmtContext* ctx) override;
    virtual std::any visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) override;
    virtual std::any visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) override;
    virtual std::any visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) override;
    virtual std::any visitSwitchCase(TzdLangParser::SwitchCaseContext* ctx) override;
    virtual std::any visitSwitchDefault(TzdLangParser::SwitchDefaultContext* ctx) override;
    virtual std::any visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) override;
    virtual std::any visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) override;
    virtual std::any visitImportStmt(TzdLangParser::ImportStmtContext* ctx) override;

    // ---- Expression visitors ----
    virtual std::any visitIntExpr(TzdLangParser::IntExprContext* ctx) override;
    virtual std::any visitFloatExpr(TzdLangParser::FloatExprContext* ctx) override;
    virtual std::any visitStringExpr(TzdLangParser::StringExprContext* ctx) override;
    virtual std::any visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) override;
    virtual std::any visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) override;
    virtual std::any visitNullExpr(TzdLangParser::NullExprContext* ctx) override;
    virtual std::any visitIdExpr(TzdLangParser::IdExprContext* ctx) override;
    virtual std::any visitParenExpr(TzdLangParser::ParenExprContext* ctx) override;
    virtual std::any visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) override;
    virtual std::any visitSuperExpr(TzdLangParser::SuperExprContext* ctx) override;
    virtual std::any visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) override;
    virtual std::any visitPrintFunction(TzdLangParser::PrintFunctionContext* ctx) override;
    virtual std::any visitCallExpr(TzdLangParser::CallExprContext* ctx) override;
    virtual std::any visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) override;
    virtual std::any visitIndexExpr(TzdLangParser::IndexExprContext* ctx) override;
    virtual std::any visitNewExpr(TzdLangParser::NewExprContext* ctx) override;
    virtual std::any visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) override;
    virtual std::any visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) override;
    virtual std::any visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) override;
    virtual std::any visitPowerExpr(TzdLangParser::PowerExprContext* ctx) override;
    virtual std::any visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) override;
    virtual std::any visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) override;
    virtual std::any visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) override;
    virtual std::any visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) override;
    virtual std::any visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) override;
    virtual std::any visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) override;
    virtual std::any visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) override;
    virtual std::any visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) override;
    virtual std::any visitCastExpr(TzdLangParser::CastExprContext* ctx) override;
    virtual std::any visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) override;

private:
    std::string exprToStr(antlr4::tree::ParseTree* tree);
    std::string escapeString(const std::string& raw);
    std::string sanitizeId(const std::string& id);

    std::ostringstream m_forwardDecls;
    std::ostringstream m_classDecls;
    std::ostringstream m_funcDecls;
    std::ostringstream m_topLevelStmts;

    std::unordered_set<std::string> m_declaredClasses;
    std::unordered_set<std::string> m_declaredFuncs;
    std::string m_currentClass;
    bool m_inClass = false;
    bool m_inMethod = false;
    bool m_cpuOnly = false;
};

} // namespace tzd

#endif // TZD_NATIVE_CODEGEN_H
