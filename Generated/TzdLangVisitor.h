
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "TzdLangParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by TzdLangParser.
 */
class  TzdLangVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by TzdLangParser.
   */
    virtual std::any visitProgram(TzdLangParser::ProgramContext *context) = 0;

    virtual std::any visitQualifiedName(TzdLangParser::QualifiedNameContext *context) = 0;

    virtual std::any visitBlockStmt(TzdLangParser::BlockStmtContext *context) = 0;

    virtual std::any visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext *context) = 0;

    virtual std::any visitAnnotationDeclStmt(TzdLangParser::AnnotationDeclStmtContext *context) = 0;

    virtual std::any visitEnumDeclStmt(TzdLangParser::EnumDeclStmtContext *context) = 0;

    virtual std::any visitFunDeclStmt(TzdLangParser::FunDeclStmtContext *context) = 0;

    virtual std::any visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext *context) = 0;

    virtual std::any visitVarDeclStmt(TzdLangParser::VarDeclStmtContext *context) = 0;

    virtual std::any visitImportStmt(TzdLangParser::ImportStmtContext *context) = 0;

    virtual std::any visitReturnStmt(TzdLangParser::ReturnStmtContext *context) = 0;

    virtual std::any visitIfStmt(TzdLangParser::IfStmtContext *context) = 0;

    virtual std::any visitWhileStmt(TzdLangParser::WhileStmtContext *context) = 0;

    virtual std::any visitForStmt(TzdLangParser::ForStmtContext *context) = 0;

    virtual std::any visitBreakStmt(TzdLangParser::BreakStmtContext *context) = 0;

    virtual std::any visitContinueStmt(TzdLangParser::ContinueStmtContext *context) = 0;

    virtual std::any visitSwitchStmt(TzdLangParser::SwitchStmtContext *context) = 0;

    virtual std::any visitTryCatchStmt(TzdLangParser::TryCatchStmtContext *context) = 0;

    virtual std::any visitThrowStmt(TzdLangParser::ThrowStmtContext *context) = 0;

    virtual std::any visitExprStmt(TzdLangParser::ExprStmtContext *context) = 0;

    virtual std::any visitEmptyStmt(TzdLangParser::EmptyStmtContext *context) = 0;

    virtual std::any visitSwitchCase(TzdLangParser::SwitchCaseContext *context) = 0;

    virtual std::any visitSwitchDefault(TzdLangParser::SwitchDefaultContext *context) = 0;

    virtual std::any visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext *context) = 0;

    virtual std::any visitEnumDeclaration(TzdLangParser::EnumDeclarationContext *context) = 0;

    virtual std::any visitEnumList(TzdLangParser::EnumListContext *context) = 0;

    virtual std::any visitClassDeclaration(TzdLangParser::ClassDeclarationContext *context) = 0;

    virtual std::any visitClassBody(TzdLangParser::ClassBodyContext *context) = 0;

    virtual std::any visitClassMember(TzdLangParser::ClassMemberContext *context) = 0;

    virtual std::any visitFieldVarDecl(TzdLangParser::FieldVarDeclContext *context) = 0;

    virtual std::any visitFieldLetDecl(TzdLangParser::FieldLetDeclContext *context) = 0;

    virtual std::any visitFieldConstDecl(TzdLangParser::FieldConstDeclContext *context) = 0;

    virtual std::any visitMethodStaticDecl(TzdLangParser::MethodStaticDeclContext *context) = 0;

    virtual std::any visitMethodAbstractDecl(TzdLangParser::MethodAbstractDeclContext *context) = 0;

    virtual std::any visitMethodDecl(TzdLangParser::MethodDeclContext *context) = 0;

    virtual std::any visitConstructorDecl(TzdLangParser::ConstructorDeclContext *context) = 0;

    virtual std::any visitAnnotationUsage(TzdLangParser::AnnotationUsageContext *context) = 0;

    virtual std::any visitAccessModifier(TzdLangParser::AccessModifierContext *context) = 0;

    virtual std::any visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext *context) = 0;

    virtual std::any visitNativeFunctionDeclaration(TzdLangParser::NativeFunctionDeclarationContext *context) = 0;

    virtual std::any visitNativeAttrList(TzdLangParser::NativeAttrListContext *context) = 0;

    virtual std::any visitNativeAttr(TzdLangParser::NativeAttrContext *context) = 0;

    virtual std::any visitNativePropKey(TzdLangParser::NativePropKeyContext *context) = 0;

    virtual std::any visitVariableDeclaration(TzdLangParser::VariableDeclarationContext *context) = 0;

    virtual std::any visitForInit(TzdLangParser::ForInitContext *context) = 0;

    virtual std::any visitImportStatement(TzdLangParser::ImportStatementContext *context) = 0;

    virtual std::any visitParamList(TzdLangParser::ParamListContext *context) = 0;

    virtual std::any visitParam(TzdLangParser::ParamContext *context) = 0;

    virtual std::any visitBlock(TzdLangParser::BlockContext *context) = 0;

    virtual std::any visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext *context) = 0;

    virtual std::any visitRelationalExpr(TzdLangParser::RelationalExprContext *context) = 0;

    virtual std::any visitAssignmentExpr(TzdLangParser::AssignmentExprContext *context) = 0;

    virtual std::any visitAtomExpr(TzdLangParser::AtomExprContext *context) = 0;

    virtual std::any visitUnaryExpr(TzdLangParser::UnaryExprContext *context) = 0;

    virtual std::any visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext *context) = 0;

    virtual std::any visitIndexExpr(TzdLangParser::IndexExprContext *context) = 0;

    virtual std::any visitPrefixExpr(TzdLangParser::PrefixExprContext *context) = 0;

    virtual std::any visitPostfixExpr(TzdLangParser::PostfixExprContext *context) = 0;

    virtual std::any visitPowerExpr(TzdLangParser::PowerExprContext *context) = 0;

    virtual std::any visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext *context) = 0;

    virtual std::any visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext *context) = 0;

    virtual std::any visitEqualityExpr(TzdLangParser::EqualityExprContext *context) = 0;

    virtual std::any visitAdditiveExpr(TzdLangParser::AdditiveExprContext *context) = 0;

    virtual std::any visitCastExpr(TzdLangParser::CastExprContext *context) = 0;

    virtual std::any visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext *context) = 0;

    virtual std::any visitStringExpr(TzdLangParser::StringExprContext *context) = 0;

    virtual std::any visitFloatExpr(TzdLangParser::FloatExprContext *context) = 0;

    virtual std::any visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext *context) = 0;

    virtual std::any visitIdExpr(TzdLangParser::IdExprContext *context) = 0;

    virtual std::any visitSuperExpr(TzdLangParser::SuperExprContext *context) = 0;

    virtual std::any visitLambdaExpr(TzdLangParser::LambdaExprContext *context) = 0;

    virtual std::any visitNullExpr(TzdLangParser::NullExprContext *context) = 0;

    virtual std::any visitPrintFunExpr(TzdLangParser::PrintFunExprContext *context) = 0;

    virtual std::any visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext *context) = 0;

    virtual std::any visitNewExpr(TzdLangParser::NewExprContext *context) = 0;

    virtual std::any visitCallExpr(TzdLangParser::CallExprContext *context) = 0;

    virtual std::any visitIntExpr(TzdLangParser::IntExprContext *context) = 0;

    virtual std::any visitParenExpr(TzdLangParser::ParenExprContext *context) = 0;

    virtual std::any visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext *context) = 0;

    virtual std::any visitClassOverrideBlock(TzdLangParser::ClassOverrideBlockContext *context) = 0;

    virtual std::any visitPrintFunction(TzdLangParser::PrintFunctionContext *context) = 0;

    virtual std::any visitExprList(TzdLangParser::ExprListContext *context) = 0;

    virtual std::any visitTypeType(TzdLangParser::TypeTypeContext *context) = 0;


};

