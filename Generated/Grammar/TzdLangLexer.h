
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  TzdLangLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, KW_VAR = 16, KW_CONST = 17, KW_LET = 18, KW_STATIC = 19, 
    KW_ABSTRACT = 20, KW_ENUM = 21, KW_IN = 22, KW_SUPER = 23, KW_NATIVE = 24, 
    KW_IMPORT = 25, KW_PRINT = 26, KW_CLASS = 27, KW_EXTENDS = 28, KW_PUBLIC = 29, 
    KW_PRIVATE = 30, KW_PROTECTED = 31, KW_FUN = 32, KW_RET = 33, KW_IF = 34, 
    KW_ELSE = 35, KW_WHILE = 36, KW_FOR = 37, KW_BREAK = 38, KW_CONTINUE = 39, 
    KW_SWITCH = 40, KW_CASE = 41, KW_DEFAULT = 42, KW_NEW = 43, KW_TRUE = 44, 
    KW_FALSE = 45, KW_NULL = 46, KW_THROW = 47, KW_TRY = 48, KW_CATCH = 49, 
    T_INT = 50, T_FLOAT = 51, T_STRING = 52, T_BOOL = 53, T_VOID = 54, T_PTR = 55, 
    T_FUNCTION = 56, INC = 57, DEC = 58, GXXX = 59, USHR_ASSIGN = 60, SHR_ASSIGN = 61, 
    SHL_ASSIGN = 62, USHR = 63, SHR = 64, SHL = 65, AND_ASSIGN = 66, OR_ASSIGN = 67, 
    XOR_ASSIGN = 68, MOD_ASSIGN = 69, PLUS_ASSIGN = 70, MIN_ASSIGN = 71, 
    MUL_ASSIGN = 72, DIV_ASSIGN = 73, EEQ = 74, NEQ = 75, GE = 76, LE = 77, 
    AND = 78, OR = 79, POW = 80, BIT_NOT = 81, BIT_AND = 82, BIT_OR = 83, 
    BIT_XOR = 84, ARROW = 85, PLUS = 86, MINUS = 87, MUL = 88, DIV = 89, 
    MOD = 90, NOT = 91, GT = 92, LT = 93, ASSIGN = 94, IDENTIFIER = 95, 
    INTEGER = 96, FLOAT = 97, STRING = 98, LINE_COMMENT = 99, BLOCK_COMMENT = 100, 
    WS = 101
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

