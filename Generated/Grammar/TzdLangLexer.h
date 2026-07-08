
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  TzdLangLexer : public antlr4::Lexer {
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

