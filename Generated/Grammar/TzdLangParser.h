
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  TzdLangParser : public antlr4::Parser {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, KW_VAR = 17, KW_CONST = 18, KW_LET = 19, KW_STATIC = 20, 
    KW_ABSTRACT = 21, KW_ENUM = 22, KW_IN = 23, KW_SUPER = 24, KW_NATIVE = 25, 
    KW_IMPORT = 26, KW_PRINT = 27, KW_CLASS = 28, KW_EXTENDS = 29, KW_PUBLIC = 30, 
    KW_PRIVATE = 31, KW_PROTECTED = 32, KW_FUN = 33, KW_RET = 34, KW_IF = 35, 
    KW_ELSE = 36, KW_WHILE = 37, KW_FOR = 38, KW_BREAK = 39, KW_CONTINUE = 40, 
    KW_SWITCH = 41, KW_CASE = 42, KW_DEFAULT = 43, KW_NEW = 44, KW_TRUE = 45, 
    KW_FALSE = 46, KW_NULL = 47, KW_THROW = 48, KW_TRY = 49, KW_CATCH = 50, 
    T_INT = 51, T_FLOAT = 52, T_STRING = 53, T_BOOL = 54, T_VOID = 55, T_PTR = 56, 
    T_FUNCTION = 57, INC = 58, DEC = 59, GXXX = 60, PLUS = 61, MINUS = 62, 
    MUL = 63, DIV = 64, MOD = 65, NOT = 66, GE = 67, LE = 68, GT = 69, LT = 70, 
    EEQ = 71, NEQ = 72, AND = 73, OR = 74, ASSIGN = 75, PLUS_ASSIGN = 76, 
    MIN_ASSIGN = 77, MUL_ASSIGN = 78, DIV_ASSIGN = 79, IDENTIFIER = 80, 
    INTEGER = 81, FLOAT = 82, STRING = 83, LINE_COMMENT = 84, BLOCK_COMMENT = 85, 
    WS = 86
  };

  enum {
    RuleProgram = 0, RuleQualifiedName = 1, RuleStatement = 2, RuleSwitchCase = 3, 
    RuleSwitchDefault = 4, RuleAnnotationDeclaration = 5, RuleEnumDeclaration = 6, 
    RuleEnumList = 7, RuleClassDeclaration = 8, RuleClassBody = 9, RuleClassMember = 10, 
    RuleMemberDecl = 11, RuleAnnotationUsage = 12, RuleAccessModifier = 13, 
    RuleFunctionDeclaration = 14, RuleNativeFunctionDeclaration = 15, RuleNativeAttrList = 16, 
    RuleNativeAttr = 17, RuleNativePropKey = 18, RuleVariableDeclaration = 19, 
    RuleForInit = 20, RuleImportStatement = 21, RuleParamList = 22, RuleParam = 23, 
    RuleBlock = 24, RuleExpression = 25, RuleAtom = 26, RuleClassOverrideBlock = 27, 
    RulePrintFunction = 28, RuleExprList = 29, RuleTypeType = 30
  };

  explicit TzdLangParser(antlr4::TokenStream *input);

  TzdLangParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~TzdLangParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class ProgramContext;
  class QualifiedNameContext;
  class StatementContext;
  class SwitchCaseContext;
  class SwitchDefaultContext;
  class AnnotationDeclarationContext;
  class EnumDeclarationContext;
  class EnumListContext;
  class ClassDeclarationContext;
  class ClassBodyContext;
  class ClassMemberContext;
  class MemberDeclContext;
  class AnnotationUsageContext;
  class AccessModifierContext;
  class FunctionDeclarationContext;
  class NativeFunctionDeclarationContext;
  class NativeAttrListContext;
  class NativeAttrContext;
  class NativePropKeyContext;
  class VariableDeclarationContext;
  class ForInitContext;
  class ImportStatementContext;
  class ParamListContext;
  class ParamContext;
  class BlockContext;
  class ExpressionContext;
  class AtomContext;
  class ClassOverrideBlockContext;
  class PrintFunctionContext;
  class ExprListContext;
  class TypeTypeContext; 

  class  ProgramContext : public antlr4::ParserRuleContext {
  public:
    ProgramContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgramContext* program();

  class  QualifiedNameContext : public antlr4::ParserRuleContext {
  public:
    QualifiedNameContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IDENTIFIER();
    antlr4::tree::TerminalNode* IDENTIFIER(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  QualifiedNameContext* qualifiedName();

  class  StatementContext : public antlr4::ParserRuleContext {
  public:
    StatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    StatementContext() = default;
    void copyFrom(StatementContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  SwitchStmtContext : public StatementContext {
  public:
    SwitchStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_SWITCH();
    ExpressionContext *expression();
    std::vector<SwitchCaseContext *> switchCase();
    SwitchCaseContext* switchCase(size_t i);
    SwitchDefaultContext *switchDefault();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ClassDeclStmtContext : public StatementContext {
  public:
    ClassDeclStmtContext(StatementContext *ctx);

    ClassDeclarationContext *classDeclaration();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BlockStmtContext : public StatementContext {
  public:
    BlockStmtContext(StatementContext *ctx);

    BlockContext *block();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NativeFunDeclStmtContext : public StatementContext {
  public:
    NativeFunDeclStmtContext(StatementContext *ctx);

    NativeFunctionDeclarationContext *nativeFunctionDeclaration();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ContinueStmtContext : public StatementContext {
  public:
    ContinueStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_CONTINUE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ImportStmtContext : public StatementContext {
  public:
    ImportStmtContext(StatementContext *ctx);

    ImportStatementContext *importStatement();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IfStmtContext : public StatementContext {
  public:
    IfStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_IF();
    ExpressionContext *expression();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);
    antlr4::tree::TerminalNode *KW_ELSE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ExprStmtContext : public StatementContext {
  public:
    ExprStmtContext(StatementContext *ctx);

    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  WhileStmtContext : public StatementContext {
  public:
    WhileStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_WHILE();
    ExpressionContext *expression();
    StatementContext *statement();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  AnnotationDeclStmtContext : public StatementContext {
  public:
    AnnotationDeclStmtContext(StatementContext *ctx);

    AnnotationDeclarationContext *annotationDeclaration();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  VarDeclStmtContext : public StatementContext {
  public:
    VarDeclStmtContext(StatementContext *ctx);

    VariableDeclarationContext *variableDeclaration();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BreakStmtContext : public StatementContext {
  public:
    BreakStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_BREAK();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  EnumDeclStmtContext : public StatementContext {
  public:
    EnumDeclStmtContext(StatementContext *ctx);

    EnumDeclarationContext *enumDeclaration();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  EmptyStmtContext : public StatementContext {
  public:
    EmptyStmtContext(StatementContext *ctx);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ReturnStmtContext : public StatementContext {
  public:
    ReturnStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_RET();
    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ForStmtContext : public StatementContext {
  public:
    ForStmtContext(StatementContext *ctx);

    TzdLangParser::ExpressionContext *cond = nullptr;
    TzdLangParser::ExpressionContext *step = nullptr;
    antlr4::tree::TerminalNode *KW_FOR();
    StatementContext *statement();
    ForInitContext *forInit();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ThrowStmtContext : public StatementContext {
  public:
    ThrowStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_THROW();
    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  FunDeclStmtContext : public StatementContext {
  public:
    FunDeclStmtContext(StatementContext *ctx);

    FunctionDeclarationContext *functionDeclaration();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  TryCatchStmtContext : public StatementContext {
  public:
    TryCatchStmtContext(StatementContext *ctx);

    antlr4::tree::TerminalNode *KW_TRY();
    std::vector<BlockContext *> block();
    BlockContext* block(size_t i);
    antlr4::tree::TerminalNode *KW_CATCH();
    antlr4::tree::TerminalNode *IDENTIFIER();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  StatementContext* statement();

  class  SwitchCaseContext : public antlr4::ParserRuleContext {
  public:
    SwitchCaseContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_CASE();
    ExpressionContext *expression();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SwitchCaseContext* switchCase();

  class  SwitchDefaultContext : public antlr4::ParserRuleContext {
  public:
    SwitchDefaultContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_DEFAULT();
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  SwitchDefaultContext* switchDefault();

  class  AnnotationDeclarationContext : public antlr4::ParserRuleContext {
  public:
    AnnotationDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_CLASS();
    antlr4::tree::TerminalNode *IDENTIFIER();
    ParamListContext *paramList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnnotationDeclarationContext* annotationDeclaration();

  class  EnumDeclarationContext : public antlr4::ParserRuleContext {
  public:
    EnumDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_ENUM();
    antlr4::tree::TerminalNode *IDENTIFIER();
    EnumListContext *enumList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumDeclarationContext* enumDeclaration();

  class  EnumListContext : public antlr4::ParserRuleContext {
  public:
    EnumListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> IDENTIFIER();
    antlr4::tree::TerminalNode* IDENTIFIER(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EnumListContext* enumList();

  class  ClassDeclarationContext : public antlr4::ParserRuleContext {
  public:
    ClassDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_CLASS();
    std::vector<QualifiedNameContext *> qualifiedName();
    QualifiedNameContext* qualifiedName(size_t i);
    ClassBodyContext *classBody();
    AnnotationUsageContext *annotationUsage();
    antlr4::tree::TerminalNode *KW_EXTENDS();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassDeclarationContext* classDeclaration();

  class  ClassBodyContext : public antlr4::ParserRuleContext {
  public:
    ClassBodyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ClassMemberContext *> classMember();
    ClassMemberContext* classMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassBodyContext* classBody();

  class  ClassMemberContext : public antlr4::ParserRuleContext {
  public:
    ClassMemberContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MemberDeclContext *memberDecl();
    AnnotationUsageContext *annotationUsage();
    AccessModifierContext *accessModifier();
    NativeFunctionDeclarationContext *nativeFunctionDeclaration();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassMemberContext* classMember();

  class  MemberDeclContext : public antlr4::ParserRuleContext {
  public:
    MemberDeclContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    MemberDeclContext() = default;
    void copyFrom(MemberDeclContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  ConstructorDeclContext : public MemberDeclContext {
  public:
    ConstructorDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *IDENTIFIER();
    BlockContext *block();
    ParamListContext *paramList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  FieldVarDeclContext : public MemberDeclContext {
  public:
    FieldVarDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *KW_VAR();
    TypeTypeContext *typeType();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *ASSIGN();
    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  MethodStaticDeclContext : public MemberDeclContext {
  public:
    MethodStaticDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *KW_STATIC();
    antlr4::tree::TerminalNode *KW_FUN();
    antlr4::tree::TerminalNode *IDENTIFIER();
    BlockContext *block();
    ParamListContext *paramList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  MethodDeclContext : public MemberDeclContext {
  public:
    MethodDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *KW_FUN();
    antlr4::tree::TerminalNode *IDENTIFIER();
    BlockContext *block();
    ParamListContext *paramList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  MethodAbstractDeclContext : public MemberDeclContext {
  public:
    MethodAbstractDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *KW_ABSTRACT();
    antlr4::tree::TerminalNode *KW_FUN();
    antlr4::tree::TerminalNode *IDENTIFIER();
    ParamListContext *paramList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  FieldConstDeclContext : public MemberDeclContext {
  public:
    FieldConstDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *KW_CONST();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *ASSIGN();
    ExpressionContext *expression();
    TypeTypeContext *typeType();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  FieldLetDeclContext : public MemberDeclContext {
  public:
    FieldLetDeclContext(MemberDeclContext *ctx);

    antlr4::tree::TerminalNode *KW_LET();
    TypeTypeContext *typeType();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *ASSIGN();
    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  MemberDeclContext* memberDecl();

  class  AnnotationUsageContext : public antlr4::ParserRuleContext {
  public:
    AnnotationUsageContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *IDENTIFIER();
    ExprListContext *exprList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AnnotationUsageContext* annotationUsage();

  class  AccessModifierContext : public antlr4::ParserRuleContext {
  public:
    AccessModifierContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_PUBLIC();
    antlr4::tree::TerminalNode *KW_PRIVATE();
    antlr4::tree::TerminalNode *KW_PROTECTED();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AccessModifierContext* accessModifier();

  class  FunctionDeclarationContext : public antlr4::ParserRuleContext {
  public:
    FunctionDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_FUN();
    antlr4::tree::TerminalNode *IDENTIFIER();
    BlockContext *block();
    AnnotationUsageContext *annotationUsage();
    ParamListContext *paramList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FunctionDeclarationContext* functionDeclaration();

  class  NativeFunctionDeclarationContext : public antlr4::ParserRuleContext {
  public:
    NativeFunctionDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_NATIVE();
    antlr4::tree::TerminalNode *KW_FUN();
    antlr4::tree::TerminalNode *IDENTIFIER();
    AnnotationUsageContext *annotationUsage();
    ParamListContext *paramList();
    NativeAttrListContext *nativeAttrList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NativeFunctionDeclarationContext* nativeFunctionDeclaration();

  class  NativeAttrListContext : public antlr4::ParserRuleContext {
  public:
    NativeAttrListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<NativeAttrContext *> nativeAttr();
    NativeAttrContext* nativeAttr(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NativeAttrListContext* nativeAttrList();

  class  NativeAttrContext : public antlr4::ParserRuleContext {
  public:
    NativeAttrContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ASSIGN();
    antlr4::tree::TerminalNode *STRING();
    antlr4::tree::TerminalNode *INTEGER();
    antlr4::tree::TerminalNode *IDENTIFIER();
    NativePropKeyContext *nativePropKey();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NativeAttrContext* nativeAttr();

  class  NativePropKeyContext : public antlr4::ParserRuleContext {
  public:
    NativePropKeyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *T_INT();
    antlr4::tree::TerminalNode *T_STRING();
    antlr4::tree::TerminalNode *T_FLOAT();
    antlr4::tree::TerminalNode *T_BOOL();
    antlr4::tree::TerminalNode *T_VOID();
    antlr4::tree::TerminalNode *T_PTR();
    antlr4::tree::TerminalNode *T_FUNCTION();
    antlr4::tree::TerminalNode *KW_RET();
    antlr4::tree::TerminalNode *KW_FUN();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  NativePropKeyContext* nativePropKey();

  class  VariableDeclarationContext : public antlr4::ParserRuleContext {
  public:
    VariableDeclarationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeTypeContext *typeType();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *ASSIGN();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *KW_VAR();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VariableDeclarationContext* variableDeclaration();

  class  ForInitContext : public antlr4::ParserRuleContext {
  public:
    ForInitContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    VariableDeclarationContext *variableDeclaration();
    ExpressionContext *expression();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ForInitContext* forInit();

  class  ImportStatementContext : public antlr4::ParserRuleContext {
  public:
    ImportStatementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_IMPORT();
    antlr4::tree::TerminalNode *STRING();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ImportStatementContext* importStatement();

  class  ParamListContext : public antlr4::ParserRuleContext {
  public:
    ParamListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ParamContext *> param();
    ParamContext* param(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParamListContext* paramList();

  class  ParamContext : public antlr4::ParserRuleContext {
  public:
    ParamContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TypeTypeContext *typeType();
    antlr4::tree::TerminalNode *IDENTIFIER();
    antlr4::tree::TerminalNode *T_INT();
    antlr4::tree::TerminalNode *T_STRING();
    antlr4::tree::TerminalNode *T_FLOAT();
    antlr4::tree::TerminalNode *T_BOOL();
    antlr4::tree::TerminalNode *T_VOID();
    antlr4::tree::TerminalNode *T_PTR();
    antlr4::tree::TerminalNode *T_FUNCTION();
    antlr4::tree::TerminalNode *KW_RET();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ParamContext* param();

  class  BlockContext : public antlr4::ParserRuleContext {
  public:
    BlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<StatementContext *> statement();
    StatementContext* statement(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  BlockContext* block();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    ExpressionContext() = default;
    void copyFrom(ExpressionContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  TypeCheckExprContext : public ExpressionContext {
  public:
    TypeCheckExprContext(ExpressionContext *ctx);

    ExpressionContext *expression();
    antlr4::tree::TerminalNode *KW_IN();
    QualifiedNameContext *qualifiedName();
    TypeTypeContext *typeType();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  RelationalExprContext : public ExpressionContext {
  public:
    RelationalExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *GT();
    antlr4::tree::TerminalNode *LT();
    antlr4::tree::TerminalNode *GE();
    antlr4::tree::TerminalNode *LE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  AssignmentExprContext : public ExpressionContext {
  public:
    AssignmentExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *ASSIGN();
    antlr4::tree::TerminalNode *PLUS_ASSIGN();
    antlr4::tree::TerminalNode *MIN_ASSIGN();
    antlr4::tree::TerminalNode *MUL_ASSIGN();
    antlr4::tree::TerminalNode *DIV_ASSIGN();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  AtomExprContext : public ExpressionContext {
  public:
    AtomExprContext(ExpressionContext *ctx);

    AtomContext *atom();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  UnaryExprContext : public ExpressionContext {
  public:
    UnaryExprContext(ExpressionContext *ctx);

    ExpressionContext *expression();
    antlr4::tree::TerminalNode *MINUS();
    antlr4::tree::TerminalNode *NOT();
    antlr4::tree::TerminalNode *GXXX();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  LogicalAndExprContext : public ExpressionContext {
  public:
    LogicalAndExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *AND();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IndexExprContext : public ExpressionContext {
  public:
    IndexExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PrefixExprContext : public ExpressionContext {
  public:
    PrefixExprContext(ExpressionContext *ctx);

    ExpressionContext *expression();
    antlr4::tree::TerminalNode *INC();
    antlr4::tree::TerminalNode *DEC();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PostfixExprContext : public ExpressionContext {
  public:
    PostfixExprContext(ExpressionContext *ctx);

    ExpressionContext *expression();
    antlr4::tree::TerminalNode *INC();
    antlr4::tree::TerminalNode *DEC();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PowerExprContext : public ExpressionContext {
  public:
    PowerExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  MultiplicativeExprContext : public ExpressionContext {
  public:
    MultiplicativeExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *MUL();
    antlr4::tree::TerminalNode *DIV();
    antlr4::tree::TerminalNode *MOD();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  LogicalOrExprContext : public ExpressionContext {
  public:
    LogicalOrExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *OR();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  EqualityExprContext : public ExpressionContext {
  public:
    EqualityExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *EEQ();
    antlr4::tree::TerminalNode *NEQ();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  AdditiveExprContext : public ExpressionContext {
  public:
    AdditiveExprContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  CastExprContext : public ExpressionContext {
  public:
    CastExprContext(ExpressionContext *ctx);

    TypeTypeContext *typeType();
    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ExpressionContext* expression();
  ExpressionContext* expression(int precedence);
  class  AtomContext : public antlr4::ParserRuleContext {
  public:
    AtomContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    AtomContext() = default;
    void copyFrom(AtomContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  BoolTrueExprContext : public AtomContext {
  public:
    BoolTrueExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *KW_TRUE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  StringExprContext : public AtomContext {
  public:
    StringExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *STRING();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  FloatExprContext : public AtomContext {
  public:
    FloatExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *FLOAT();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BoolFalseExprContext : public AtomContext {
  public:
    BoolFalseExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *KW_FALSE();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IdExprContext : public AtomContext {
  public:
    IdExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *IDENTIFIER();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  SuperExprContext : public AtomContext {
  public:
    SuperExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *KW_SUPER();
    ExprListContext *exprList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  LambdaExprContext : public AtomContext {
  public:
    LambdaExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *KW_FUN();
    BlockContext *block();
    ParamListContext *paramList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NullExprContext : public AtomContext {
  public:
    NullExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *KW_NULL();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PrintFunExprContext : public AtomContext {
  public:
    PrintFunExprContext(AtomContext *ctx);

    PrintFunctionContext *printFunction();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ArrayLiteralExprContext : public AtomContext {
  public:
    ArrayLiteralExprContext(AtomContext *ctx);

    ExprListContext *exprList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  NewExprContext : public AtomContext {
  public:
    NewExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *KW_NEW();
    QualifiedNameContext *qualifiedName();
    ExprListContext *exprList();
    ClassOverrideBlockContext *classOverrideBlock();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  CallExprContext : public AtomContext {
  public:
    CallExprContext(AtomContext *ctx);

    AtomContext *atom();
    ExprListContext *exprList();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  IntExprContext : public AtomContext {
  public:
    IntExprContext(AtomContext *ctx);

    antlr4::tree::TerminalNode *INTEGER();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ParenExprContext : public AtomContext {
  public:
    ParenExprContext(AtomContext *ctx);

    ExpressionContext *expression();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  MemberAccessExprContext : public AtomContext {
  public:
    MemberAccessExprContext(AtomContext *ctx);

    AtomContext *atom();
    antlr4::tree::TerminalNode *IDENTIFIER();

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  AtomContext* atom();
  AtomContext* atom(int precedence);
  class  ClassOverrideBlockContext : public antlr4::ParserRuleContext {
  public:
    ClassOverrideBlockContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ClassMemberContext *> classMember();
    ClassMemberContext* classMember(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClassOverrideBlockContext* classOverrideBlock();

  class  PrintFunctionContext : public antlr4::ParserRuleContext {
  public:
    PrintFunctionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *KW_PRINT();
    ExprListContext *exprList();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  PrintFunctionContext* printFunction();

  class  ExprListContext : public antlr4::ParserRuleContext {
  public:
    ExprListContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExprListContext* exprList();

  class  TypeTypeContext : public antlr4::ParserRuleContext {
  public:
    TypeTypeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *T_INT();
    antlr4::tree::TerminalNode *T_FLOAT();
    antlr4::tree::TerminalNode *T_STRING();
    antlr4::tree::TerminalNode *T_BOOL();
    antlr4::tree::TerminalNode *T_VOID();
    antlr4::tree::TerminalNode *T_PTR();
    antlr4::tree::TerminalNode *T_FUNCTION();
    QualifiedNameContext *qualifiedName();
    TypeTypeContext *typeType();


    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TypeTypeContext* typeType();
  TypeTypeContext* typeType(int precedence);

  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool expressionSempred(ExpressionContext *_localctx, size_t predicateIndex);
  bool atomSempred(AtomContext *_localctx, size_t predicateIndex);
  bool typeTypeSempred(TypeTypeContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

