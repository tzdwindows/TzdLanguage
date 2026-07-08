
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "TzdLangVisitor.h"


/**
 * This class provides an empty implementation of TzdLangVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  TzdLangBaseVisitor : public TzdLangVisitor {
public:

  virtual std::any visitProgram(TzdLangParser::ProgramContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitQualifiedName(TzdLangParser::QualifiedNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlockStmt(TzdLangParser::BlockStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnnotationDeclStmt(TzdLangParser::AnnotationDeclStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumDeclStmt(TzdLangParser::EnumDeclStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunDeclStmt(TzdLangParser::FunDeclStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarDeclStmt(TzdLangParser::VarDeclStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportStmt(TzdLangParser::ImportStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitReturnStmt(TzdLangParser::ReturnStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIfStmt(TzdLangParser::IfStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitWhileStmt(TzdLangParser::WhileStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForStmt(TzdLangParser::ForStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBreakStmt(TzdLangParser::BreakStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitContinueStmt(TzdLangParser::ContinueStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwitchStmt(TzdLangParser::SwitchStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTryCatchStmt(TzdLangParser::TryCatchStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitThrowStmt(TzdLangParser::ThrowStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprStmt(TzdLangParser::ExprStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEmptyStmt(TzdLangParser::EmptyStmtContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwitchCase(TzdLangParser::SwitchCaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSwitchDefault(TzdLangParser::SwitchDefaultContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumDeclaration(TzdLangParser::EnumDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumList(TzdLangParser::EnumListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassDeclaration(TzdLangParser::ClassDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassBody(TzdLangParser::ClassBodyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassMember(TzdLangParser::ClassMemberContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldVarDecl(TzdLangParser::FieldVarDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldLetDecl(TzdLangParser::FieldLetDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFieldConstDecl(TzdLangParser::FieldConstDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMethodStaticDecl(TzdLangParser::MethodStaticDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMethodAbstractDecl(TzdLangParser::MethodAbstractDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMethodDecl(TzdLangParser::MethodDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstructorDecl(TzdLangParser::ConstructorDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnnotationUsage(TzdLangParser::AnnotationUsageContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAccessModifier(TzdLangParser::AccessModifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNativeFunctionDeclaration(TzdLangParser::NativeFunctionDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNativeAttrList(TzdLangParser::NativeAttrListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNativeAttr(TzdLangParser::NativeAttrContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNativePropKey(TzdLangParser::NativePropKeyContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariableDeclaration(TzdLangParser::VariableDeclarationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitForInit(TzdLangParser::ForInitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitImportStatement(TzdLangParser::ImportStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParamList(TzdLangParser::ParamListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParam(TzdLangParser::ParamContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBlock(TzdLangParser::BlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRelationalExpr(TzdLangParser::RelationalExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAssignmentExpr(TzdLangParser::AssignmentExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAtomExpr(TzdLangParser::AtomExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnaryExpr(TzdLangParser::UnaryExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIndexExpr(TzdLangParser::IndexExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrefixExpr(TzdLangParser::PrefixExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostfixExpr(TzdLangParser::PostfixExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPowerExpr(TzdLangParser::PowerExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEqualityExpr(TzdLangParser::EqualityExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAdditiveExpr(TzdLangParser::AdditiveExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCastExpr(TzdLangParser::CastExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStringExpr(TzdLangParser::StringExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFloatExpr(TzdLangParser::FloatExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIdExpr(TzdLangParser::IdExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSuperExpr(TzdLangParser::SuperExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLambdaExpr(TzdLangParser::LambdaExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNullExpr(TzdLangParser::NullExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrintFunExpr(TzdLangParser::PrintFunExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNewExpr(TzdLangParser::NewExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCallExpr(TzdLangParser::CallExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntExpr(TzdLangParser::IntExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParenExpr(TzdLangParser::ParenExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitClassOverrideBlock(TzdLangParser::ClassOverrideBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPrintFunction(TzdLangParser::PrintFunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExprList(TzdLangParser::ExprListContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTypeType(TzdLangParser::TypeTypeContext *ctx) override {
    return visitChildren(ctx);
  }


};

