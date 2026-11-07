
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  TzdLangLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    KW_VAR = 15, KW_CONST = 16, KW_LET = 17, KW_STATIC = 18, KW_ABSTRACT = 19, 
    KW_ENUM = 20, KW_IN = 21, KW_SUPER = 22, KW_NATIVE = 23, KW_IMPORT = 24, 
    KW_PRINT = 25, KW_CLASS = 26, KW_EXTENDS = 27, KW_PUBLIC = 28, KW_PRIVATE = 29, 
    KW_PROTECTED = 30, KW_FUN = 31, KW_RET = 32, KW_IF = 33, KW_ELSE = 34, 
    KW_WHILE = 35, KW_FOR = 36, KW_BREAK = 37, KW_CONTINUE = 38, KW_SWITCH = 39, 
    KW_CASE = 40, KW_DEFAULT = 41, KW_NEW = 42, KW_TRUE = 43, KW_FALSE = 44, 
    KW_NULL = 45, KW_THROW = 46, KW_TRY = 47, KW_CATCH = 48, T_INT = 49, 
    T_FLOAT = 50, T_STRING = 51, T_BOOL = 52, T_VOID = 53, T_PTR = 54, T_FUNCTION = 55, 
    INC = 56, DEC = 57, GXXX = 58, USHR_ASSIGN = 59, SHR_ASSIGN = 60, SHL_ASSIGN = 61, 
    USHR = 62, SHR = 63, SHL = 64, AND_ASSIGN = 65, OR_ASSIGN = 66, XOR_ASSIGN = 67, 
    MOD_ASSIGN = 68, PLUS_ASSIGN = 69, MIN_ASSIGN = 70, MUL_ASSIGN = 71, 
    DIV_ASSIGN = 72, EEQ = 73, NEQ = 74, GE = 75, LE = 76, AND = 77, OR = 78, 
    POW = 79, BIT_NOT = 80, BIT_AND = 81, BIT_OR = 82, BIT_XOR = 83, ARROW = 84, 
    PLUS = 85, MINUS = 86, MUL = 87, DIV = 88, MOD = 89, NOT = 90, GT = 91, 
    LT = 92, ASSIGN = 93, ELLIPSIS = 94, IDENTIFIER = 95, INTEGER = 96, 
    FLOAT = 97, STRING = 98, LINE_COMMENT = 99, BLOCK_COMMENT = 100, WS = 101
  };

  explicit TzdLangLexer(antlr4::CharStream *input);

  ~TzdLangLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

