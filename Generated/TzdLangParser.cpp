
// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1


#include "TzdLangVisitor.h"

#include "TzdLangParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct TzdLangParserStaticData final {
  TzdLangParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  TzdLangParserStaticData(const TzdLangParserStaticData&) = delete;
  TzdLangParserStaticData(TzdLangParserStaticData&&) = delete;
  TzdLangParserStaticData& operator=(const TzdLangParserStaticData&) = delete;
  TzdLangParserStaticData& operator=(TzdLangParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag tzdlangParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
TzdLangParserStaticData *tzdlangParserStaticData = nullptr;

void tzdlangParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (tzdlangParserStaticData != nullptr) {
    return;
  }
#else
  assert(tzdlangParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<TzdLangParserStaticData>(
    std::vector<std::string>{
      "program", "qualifiedName", "statement", "switchCase", "switchDefault", 
      "annotationDeclaration", "enumDeclaration", "enumList", "classDeclaration", 
      "classBody", "classMember", "memberDecl", "annotationUsage", "accessModifier", 
      "functionDeclaration", "nativeFunctionDeclaration", "nativeAttrList", 
      "nativeAttr", "nativePropKey", "variableDeclaration", "forInit", "forEachInit", 
      "importStatement", "paramList", "param", "block", "expression", "atom", 
      "classOverrideBlock", "printFunction", "exprList", "mapEntryList", 
      "mapEntry", "mapKey", "typeType"
    },
    std::vector<std::string>{
      "", "'.'", "';'", "'('", "')'", "':'", "'{'", "'}'", "'@'", "','", 
      "'type'", "'dll'", "'prototype'", "'['", "']'", "'var'", "'const'", 
      "'let'", "'static'", "'abstract'", "'enum'", "'in'", "'super'", "'native'", 
      "'import'", "", "'class'", "'extends'", "'public'", "'private'", "'protected'", 
      "'fun'", "", "'if'", "'else'", "'while'", "'for'", "'break'", "'continue'", 
      "'switch'", "'case'", "'default'", "'new'", "'true'", "'false'", "'null'", 
      "'throw'", "'try'", "'catch'", "'int'", "'float'", "'string'", "'bool'", 
      "'void'", "", "", "'++'", "'--'", "'gxxx'", "'>>>='", "'>>='", "'<<='", 
      "'>>>'", "'>>'", "'<<'", "'&='", "'|='", "'^='", "'%='", "'+='", "'-='", 
      "'*='", "'/='", "'=='", "'!='", "'>='", "'<='", "'&&'", "'||'", "'**'", 
      "'~'", "'&'", "'|'", "'^'", "'->'", "'+'", "'-'", "'*'", "'/'", "'%'", 
      "'!'", "'>'", "'<'", "'='", "'...'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "KW_VAR", 
      "KW_CONST", "KW_LET", "KW_STATIC", "KW_ABSTRACT", "KW_ENUM", "KW_IN", 
      "KW_SUPER", "KW_NATIVE", "KW_IMPORT", "KW_PRINT", "KW_CLASS", "KW_EXTENDS", 
      "KW_PUBLIC", "KW_PRIVATE", "KW_PROTECTED", "KW_FUN", "KW_RET", "KW_IF", 
      "KW_ELSE", "KW_WHILE", "KW_FOR", "KW_BREAK", "KW_CONTINUE", "KW_SWITCH", 
      "KW_CASE", "KW_DEFAULT", "KW_NEW", "KW_TRUE", "KW_FALSE", "KW_NULL", 
      "KW_THROW", "KW_TRY", "KW_CATCH", "T_INT", "T_FLOAT", "T_STRING", 
      "T_BOOL", "T_VOID", "T_PTR", "T_FUNCTION", "INC", "DEC", "GXXX", "USHR_ASSIGN", 
      "SHR_ASSIGN", "SHL_ASSIGN", "USHR", "SHR", "SHL", "AND_ASSIGN", "OR_ASSIGN", 
      "XOR_ASSIGN", "MOD_ASSIGN", "PLUS_ASSIGN", "MIN_ASSIGN", "MUL_ASSIGN", 
      "DIV_ASSIGN", "EEQ", "NEQ", "GE", "LE", "AND", "OR", "POW", "BIT_NOT", 
      "BIT_AND", "BIT_OR", "BIT_XOR", "ARROW", "PLUS", "MINUS", "MUL", "DIV", 
      "MOD", "NOT", "GT", "LT", "ASSIGN", "ELLIPSIS", "IDENTIFIER", "INTEGER", 
      "FLOAT", "STRING", "LINE_COMMENT", "BLOCK_COMMENT", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,101,678,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,1,0,5,
  	0,72,8,0,10,0,12,0,75,9,0,1,0,1,0,1,1,1,1,1,1,5,1,82,8,1,10,1,12,1,85,
  	9,1,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,99,8,2,1,2,1,
  	2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,109,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
  	1,2,3,2,120,8,2,1,2,1,2,3,2,124,8,2,1,2,1,2,3,2,128,8,2,1,2,1,2,1,2,1,
  	2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,5,2,
  	150,8,2,10,2,12,2,153,9,2,1,2,3,2,156,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
  	1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,176,8,2,1,3,1,3,1,3,1,
  	3,5,3,182,8,3,10,3,12,3,185,9,3,1,4,1,4,1,4,5,4,190,8,4,10,4,12,4,193,
  	9,4,1,5,1,5,1,5,1,5,1,5,3,5,200,8,5,1,5,1,5,1,6,1,6,1,6,1,6,3,6,208,8,
  	6,1,6,1,6,1,7,1,7,1,7,5,7,215,8,7,10,7,12,7,218,9,7,1,8,3,8,221,8,8,1,
  	8,1,8,1,8,1,8,3,8,227,8,8,1,8,1,8,1,8,1,8,1,8,3,8,234,8,8,1,8,1,8,1,8,
  	1,8,3,8,240,8,8,1,8,1,8,1,8,1,8,3,8,246,8,8,1,9,5,9,249,8,9,10,9,12,9,
  	252,9,9,1,10,3,10,255,8,10,1,10,3,10,258,8,10,1,10,1,10,3,10,262,8,10,
  	1,11,1,11,1,11,1,11,1,11,3,11,269,8,11,1,11,1,11,1,11,1,11,1,11,1,11,
  	1,11,3,11,278,8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,286,8,11,1,11,1,
  	11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,297,8,11,1,11,1,11,1,11,3,
  	11,302,8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,310,8,11,1,11,1,11,1,11,
  	3,11,315,8,11,1,11,1,11,1,11,1,11,1,11,3,11,322,8,11,1,11,1,11,1,11,3,
  	11,327,8,11,1,11,1,11,1,11,1,11,3,11,333,8,11,1,11,1,11,3,11,337,8,11,
  	1,12,1,12,1,12,1,12,3,12,343,8,12,1,12,3,12,346,8,12,1,13,1,13,1,14,3,
  	14,351,8,14,1,14,1,14,1,14,1,14,3,14,357,8,14,1,14,1,14,1,14,3,14,362,
  	8,14,1,14,1,14,1,15,3,15,367,8,15,1,15,1,15,1,15,1,15,1,15,3,15,374,8,
  	15,1,15,1,15,1,15,3,15,379,8,15,1,15,1,15,3,15,383,8,15,1,15,1,15,1,15,
  	1,16,1,16,1,16,5,16,391,8,16,10,16,12,16,394,9,16,1,17,1,17,3,17,398,
  	8,17,1,17,1,17,1,17,1,18,1,18,1,19,1,19,1,19,1,19,3,19,409,8,19,1,19,
  	1,19,1,19,1,19,3,19,415,8,19,1,19,1,19,1,19,1,19,1,19,3,19,422,8,19,3,
  	19,424,8,19,1,20,1,20,3,20,428,8,20,1,21,1,21,3,21,432,8,21,1,21,1,21,
  	1,22,1,22,1,22,1,22,1,23,1,23,1,23,5,23,443,8,23,10,23,12,23,446,9,23,
  	1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,3,24,460,
  	8,24,3,24,462,8,24,1,25,1,25,5,25,466,8,25,10,25,12,25,469,9,25,1,25,
  	1,25,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,484,
  	8,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,
  	1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,
  	1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,
  	1,26,1,26,1,26,1,26,1,26,1,26,3,26,533,8,26,5,26,535,8,26,10,26,12,26,
  	538,9,26,1,27,1,27,1,27,1,27,1,27,1,27,1,27,3,27,547,8,27,1,27,1,27,1,
  	27,3,27,552,8,27,1,27,1,27,1,27,3,27,557,8,27,1,27,1,27,1,27,1,27,1,27,
  	1,27,1,27,1,27,1,27,1,27,1,27,3,27,570,8,27,1,27,1,27,1,27,1,27,1,27,
  	1,27,3,27,578,8,27,1,27,1,27,3,27,582,8,27,1,27,1,27,1,27,3,27,587,8,
  	27,1,27,1,27,1,27,3,27,592,8,27,1,27,3,27,595,8,27,1,27,1,27,1,27,3,27,
  	600,8,27,1,27,1,27,1,27,1,27,5,27,606,8,27,10,27,12,27,609,9,27,1,28,
  	1,28,5,28,613,8,28,10,28,12,28,616,9,28,1,28,1,28,1,29,1,29,1,29,3,29,
  	623,8,29,1,29,1,29,1,30,1,30,1,30,5,30,630,8,30,10,30,12,30,633,9,30,
  	1,31,1,31,1,31,5,31,638,8,31,10,31,12,31,641,9,31,1,31,3,31,644,8,31,
  	1,32,1,32,1,32,1,32,1,33,1,33,1,33,1,33,1,33,1,33,1,33,3,33,657,8,33,
  	1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,3,34,668,8,34,1,34,1,34,
  	1,34,5,34,673,8,34,10,34,12,34,676,9,34,1,34,0,3,52,54,68,35,0,2,4,6,
  	8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,
  	56,58,60,62,64,66,68,0,12,1,0,28,30,2,0,96,96,98,98,3,0,10,12,31,32,49,
  	55,3,0,32,32,49,55,95,95,1,0,56,57,4,0,58,58,80,80,85,86,90,90,1,0,87,
  	89,1,0,85,86,1,0,62,64,2,0,75,76,91,92,1,0,73,74,3,0,59,61,65,72,93,93,
  	785,0,73,1,0,0,0,2,78,1,0,0,0,4,175,1,0,0,0,6,177,1,0,0,0,8,186,1,0,0,
  	0,10,194,1,0,0,0,12,203,1,0,0,0,14,211,1,0,0,0,16,245,1,0,0,0,18,250,
  	1,0,0,0,20,261,1,0,0,0,22,336,1,0,0,0,24,338,1,0,0,0,26,347,1,0,0,0,28,
  	350,1,0,0,0,30,366,1,0,0,0,32,387,1,0,0,0,34,397,1,0,0,0,36,402,1,0,0,
  	0,38,423,1,0,0,0,40,427,1,0,0,0,42,431,1,0,0,0,44,435,1,0,0,0,46,439,
  	1,0,0,0,48,461,1,0,0,0,50,463,1,0,0,0,52,483,1,0,0,0,54,594,1,0,0,0,56,
  	610,1,0,0,0,58,619,1,0,0,0,60,626,1,0,0,0,62,634,1,0,0,0,64,645,1,0,0,
  	0,66,656,1,0,0,0,68,667,1,0,0,0,70,72,3,4,2,0,71,70,1,0,0,0,72,75,1,0,
  	0,0,73,71,1,0,0,0,73,74,1,0,0,0,74,76,1,0,0,0,75,73,1,0,0,0,76,77,5,0,
  	0,1,77,1,1,0,0,0,78,83,5,95,0,0,79,80,5,1,0,0,80,82,5,95,0,0,81,79,1,
  	0,0,0,82,85,1,0,0,0,83,81,1,0,0,0,83,84,1,0,0,0,84,3,1,0,0,0,85,83,1,
  	0,0,0,86,176,3,50,25,0,87,176,3,16,8,0,88,176,3,10,5,0,89,176,3,12,6,
  	0,90,176,3,28,14,0,91,176,3,30,15,0,92,93,3,38,19,0,93,94,5,2,0,0,94,
  	176,1,0,0,0,95,176,3,44,22,0,96,98,5,32,0,0,97,99,3,52,26,0,98,97,1,0,
  	0,0,98,99,1,0,0,0,99,100,1,0,0,0,100,176,5,2,0,0,101,102,5,33,0,0,102,
  	103,5,3,0,0,103,104,3,52,26,0,104,105,5,4,0,0,105,108,3,4,2,0,106,107,
  	5,34,0,0,107,109,3,4,2,0,108,106,1,0,0,0,108,109,1,0,0,0,109,176,1,0,
  	0,0,110,111,5,35,0,0,111,112,5,3,0,0,112,113,3,52,26,0,113,114,5,4,0,
  	0,114,115,3,4,2,0,115,176,1,0,0,0,116,117,5,36,0,0,117,119,5,3,0,0,118,
  	120,3,40,20,0,119,118,1,0,0,0,119,120,1,0,0,0,120,121,1,0,0,0,121,123,
  	5,2,0,0,122,124,3,52,26,0,123,122,1,0,0,0,123,124,1,0,0,0,124,125,1,0,
  	0,0,125,127,5,2,0,0,126,128,3,52,26,0,127,126,1,0,0,0,127,128,1,0,0,0,
  	128,129,1,0,0,0,129,130,5,4,0,0,130,176,3,4,2,0,131,132,5,36,0,0,132,
  	133,5,3,0,0,133,134,3,42,21,0,134,135,5,5,0,0,135,136,3,52,26,0,136,137,
  	5,4,0,0,137,138,3,4,2,0,138,176,1,0,0,0,139,140,5,37,0,0,140,176,5,2,
  	0,0,141,142,5,38,0,0,142,176,5,2,0,0,143,144,5,39,0,0,144,145,5,3,0,0,
  	145,146,3,52,26,0,146,147,5,4,0,0,147,151,5,6,0,0,148,150,3,6,3,0,149,
  	148,1,0,0,0,150,153,1,0,0,0,151,149,1,0,0,0,151,152,1,0,0,0,152,155,1,
  	0,0,0,153,151,1,0,0,0,154,156,3,8,4,0,155,154,1,0,0,0,155,156,1,0,0,0,
  	156,157,1,0,0,0,157,158,5,7,0,0,158,176,1,0,0,0,159,160,5,47,0,0,160,
  	161,3,50,25,0,161,162,5,48,0,0,162,163,5,3,0,0,163,164,5,95,0,0,164,165,
  	5,4,0,0,165,166,3,50,25,0,166,176,1,0,0,0,167,168,5,46,0,0,168,169,3,
  	52,26,0,169,170,5,2,0,0,170,176,1,0,0,0,171,172,3,52,26,0,172,173,5,2,
  	0,0,173,176,1,0,0,0,174,176,5,2,0,0,175,86,1,0,0,0,175,87,1,0,0,0,175,
  	88,1,0,0,0,175,89,1,0,0,0,175,90,1,0,0,0,175,91,1,0,0,0,175,92,1,0,0,
  	0,175,95,1,0,0,0,175,96,1,0,0,0,175,101,1,0,0,0,175,110,1,0,0,0,175,116,
  	1,0,0,0,175,131,1,0,0,0,175,139,1,0,0,0,175,141,1,0,0,0,175,143,1,0,0,
  	0,175,159,1,0,0,0,175,167,1,0,0,0,175,171,1,0,0,0,175,174,1,0,0,0,176,
  	5,1,0,0,0,177,178,5,40,0,0,178,179,3,52,26,0,179,183,5,5,0,0,180,182,
  	3,4,2,0,181,180,1,0,0,0,182,185,1,0,0,0,183,181,1,0,0,0,183,184,1,0,0,
  	0,184,7,1,0,0,0,185,183,1,0,0,0,186,187,5,41,0,0,187,191,5,5,0,0,188,
  	190,3,4,2,0,189,188,1,0,0,0,190,193,1,0,0,0,191,189,1,0,0,0,191,192,1,
  	0,0,0,192,9,1,0,0,0,193,191,1,0,0,0,194,195,5,26,0,0,195,196,5,8,0,0,
  	196,197,5,95,0,0,197,199,5,3,0,0,198,200,3,46,23,0,199,198,1,0,0,0,199,
  	200,1,0,0,0,200,201,1,0,0,0,201,202,5,4,0,0,202,11,1,0,0,0,203,204,5,
  	20,0,0,204,205,5,95,0,0,205,207,5,6,0,0,206,208,3,14,7,0,207,206,1,0,
  	0,0,207,208,1,0,0,0,208,209,1,0,0,0,209,210,5,7,0,0,210,13,1,0,0,0,211,
  	216,5,95,0,0,212,213,5,9,0,0,213,215,5,95,0,0,214,212,1,0,0,0,215,218,
  	1,0,0,0,216,214,1,0,0,0,216,217,1,0,0,0,217,15,1,0,0,0,218,216,1,0,0,
  	0,219,221,3,24,12,0,220,219,1,0,0,0,220,221,1,0,0,0,221,222,1,0,0,0,222,
  	223,5,26,0,0,223,226,3,2,1,0,224,225,5,5,0,0,225,227,3,2,1,0,226,224,
  	1,0,0,0,226,227,1,0,0,0,227,228,1,0,0,0,228,229,5,6,0,0,229,230,3,18,
  	9,0,230,231,5,7,0,0,231,246,1,0,0,0,232,234,3,24,12,0,233,232,1,0,0,0,
  	233,234,1,0,0,0,234,235,1,0,0,0,235,236,5,26,0,0,236,239,3,2,1,0,237,
  	238,5,27,0,0,238,240,3,2,1,0,239,237,1,0,0,0,239,240,1,0,0,0,240,241,
  	1,0,0,0,241,242,5,6,0,0,242,243,3,18,9,0,243,244,5,7,0,0,244,246,1,0,
  	0,0,245,220,1,0,0,0,245,233,1,0,0,0,246,17,1,0,0,0,247,249,3,20,10,0,
  	248,247,1,0,0,0,249,252,1,0,0,0,250,248,1,0,0,0,250,251,1,0,0,0,251,19,
  	1,0,0,0,252,250,1,0,0,0,253,255,3,24,12,0,254,253,1,0,0,0,254,255,1,0,
  	0,0,255,257,1,0,0,0,256,258,3,26,13,0,257,256,1,0,0,0,257,258,1,0,0,0,
  	258,259,1,0,0,0,259,262,3,22,11,0,260,262,3,30,15,0,261,254,1,0,0,0,261,
  	260,1,0,0,0,262,21,1,0,0,0,263,264,5,15,0,0,264,265,3,68,34,0,265,268,
  	5,95,0,0,266,267,5,93,0,0,267,269,3,52,26,0,268,266,1,0,0,0,268,269,1,
  	0,0,0,269,270,1,0,0,0,270,271,5,2,0,0,271,337,1,0,0,0,272,273,5,17,0,
  	0,273,274,3,68,34,0,274,277,5,95,0,0,275,276,5,93,0,0,276,278,3,52,26,
  	0,277,275,1,0,0,0,277,278,1,0,0,0,278,279,1,0,0,0,279,280,5,2,0,0,280,
  	337,1,0,0,0,281,282,5,16,0,0,282,285,5,95,0,0,283,284,5,5,0,0,284,286,
  	3,68,34,0,285,283,1,0,0,0,285,286,1,0,0,0,286,287,1,0,0,0,287,288,5,93,
  	0,0,288,289,3,52,26,0,289,290,5,2,0,0,290,337,1,0,0,0,291,292,5,18,0,
  	0,292,293,5,31,0,0,293,294,5,95,0,0,294,296,5,3,0,0,295,297,3,46,23,0,
  	296,295,1,0,0,0,296,297,1,0,0,0,297,298,1,0,0,0,298,301,5,4,0,0,299,300,
  	5,84,0,0,300,302,3,68,34,0,301,299,1,0,0,0,301,302,1,0,0,0,302,303,1,
  	0,0,0,303,337,3,50,25,0,304,305,5,19,0,0,305,306,5,31,0,0,306,307,5,95,
  	0,0,307,309,5,3,0,0,308,310,3,46,23,0,309,308,1,0,0,0,309,310,1,0,0,0,
  	310,311,1,0,0,0,311,314,5,4,0,0,312,313,5,84,0,0,313,315,3,68,34,0,314,
  	312,1,0,0,0,314,315,1,0,0,0,315,316,1,0,0,0,316,337,5,2,0,0,317,318,5,
  	31,0,0,318,319,5,95,0,0,319,321,5,3,0,0,320,322,3,46,23,0,321,320,1,0,
  	0,0,321,322,1,0,0,0,322,323,1,0,0,0,323,326,5,4,0,0,324,325,5,84,0,0,
  	325,327,3,68,34,0,326,324,1,0,0,0,326,327,1,0,0,0,327,328,1,0,0,0,328,
  	337,3,50,25,0,329,330,5,95,0,0,330,332,5,3,0,0,331,333,3,46,23,0,332,
  	331,1,0,0,0,332,333,1,0,0,0,333,334,1,0,0,0,334,335,5,4,0,0,335,337,3,
  	50,25,0,336,263,1,0,0,0,336,272,1,0,0,0,336,281,1,0,0,0,336,291,1,0,0,
  	0,336,304,1,0,0,0,336,317,1,0,0,0,336,329,1,0,0,0,337,23,1,0,0,0,338,
  	339,5,8,0,0,339,345,5,95,0,0,340,342,5,3,0,0,341,343,3,60,30,0,342,341,
  	1,0,0,0,342,343,1,0,0,0,343,344,1,0,0,0,344,346,5,4,0,0,345,340,1,0,0,
  	0,345,346,1,0,0,0,346,25,1,0,0,0,347,348,7,0,0,0,348,27,1,0,0,0,349,351,
  	3,24,12,0,350,349,1,0,0,0,350,351,1,0,0,0,351,352,1,0,0,0,352,353,5,31,
  	0,0,353,354,5,95,0,0,354,356,5,3,0,0,355,357,3,46,23,0,356,355,1,0,0,
  	0,356,357,1,0,0,0,357,358,1,0,0,0,358,361,5,4,0,0,359,360,5,84,0,0,360,
  	362,3,68,34,0,361,359,1,0,0,0,361,362,1,0,0,0,362,363,1,0,0,0,363,364,
  	3,50,25,0,364,29,1,0,0,0,365,367,3,24,12,0,366,365,1,0,0,0,366,367,1,
  	0,0,0,367,368,1,0,0,0,368,369,5,23,0,0,369,370,5,31,0,0,370,371,5,95,
  	0,0,371,373,5,3,0,0,372,374,3,46,23,0,373,372,1,0,0,0,373,374,1,0,0,0,
  	374,375,1,0,0,0,375,378,5,4,0,0,376,377,5,84,0,0,377,379,3,68,34,0,378,
  	376,1,0,0,0,378,379,1,0,0,0,379,380,1,0,0,0,380,382,5,3,0,0,381,383,3,
  	32,16,0,382,381,1,0,0,0,382,383,1,0,0,0,383,384,1,0,0,0,384,385,5,4,0,
  	0,385,386,5,2,0,0,386,31,1,0,0,0,387,392,3,34,17,0,388,389,5,9,0,0,389,
  	391,3,34,17,0,390,388,1,0,0,0,391,394,1,0,0,0,392,390,1,0,0,0,392,393,
  	1,0,0,0,393,33,1,0,0,0,394,392,1,0,0,0,395,398,5,95,0,0,396,398,3,36,
  	18,0,397,395,1,0,0,0,397,396,1,0,0,0,398,399,1,0,0,0,399,400,5,93,0,0,
  	400,401,7,1,0,0,401,35,1,0,0,0,402,403,7,2,0,0,403,37,1,0,0,0,404,405,
  	3,68,34,0,405,408,5,95,0,0,406,407,5,93,0,0,407,409,3,52,26,0,408,406,
  	1,0,0,0,408,409,1,0,0,0,409,424,1,0,0,0,410,411,5,15,0,0,411,414,5,95,
  	0,0,412,413,5,93,0,0,413,415,3,52,26,0,414,412,1,0,0,0,414,415,1,0,0,
  	0,415,424,1,0,0,0,416,417,5,95,0,0,417,418,5,5,0,0,418,421,3,68,34,0,
  	419,420,5,93,0,0,420,422,3,52,26,0,421,419,1,0,0,0,421,422,1,0,0,0,422,
  	424,1,0,0,0,423,404,1,0,0,0,423,410,1,0,0,0,423,416,1,0,0,0,424,39,1,
  	0,0,0,425,428,3,38,19,0,426,428,3,52,26,0,427,425,1,0,0,0,427,426,1,0,
  	0,0,428,41,1,0,0,0,429,432,3,68,34,0,430,432,5,15,0,0,431,429,1,0,0,0,
  	431,430,1,0,0,0,431,432,1,0,0,0,432,433,1,0,0,0,433,434,5,95,0,0,434,
  	43,1,0,0,0,435,436,5,24,0,0,436,437,5,98,0,0,437,438,5,2,0,0,438,45,1,
  	0,0,0,439,444,3,48,24,0,440,441,5,9,0,0,441,443,3,48,24,0,442,440,1,0,
  	0,0,443,446,1,0,0,0,444,442,1,0,0,0,444,445,1,0,0,0,445,47,1,0,0,0,446,
  	444,1,0,0,0,447,448,3,68,34,0,448,449,5,94,0,0,449,450,5,95,0,0,450,462,
  	1,0,0,0,451,452,5,94,0,0,452,462,5,95,0,0,453,454,3,68,34,0,454,455,5,
  	95,0,0,455,462,1,0,0,0,456,459,7,3,0,0,457,458,5,5,0,0,458,460,3,68,34,
  	0,459,457,1,0,0,0,459,460,1,0,0,0,460,462,1,0,0,0,461,447,1,0,0,0,461,
  	451,1,0,0,0,461,453,1,0,0,0,461,456,1,0,0,0,462,49,1,0,0,0,463,467,5,
  	6,0,0,464,466,3,4,2,0,465,464,1,0,0,0,466,469,1,0,0,0,467,465,1,0,0,0,
  	467,468,1,0,0,0,468,470,1,0,0,0,469,467,1,0,0,0,470,471,5,7,0,0,471,51,
  	1,0,0,0,472,473,6,26,-1,0,473,474,5,3,0,0,474,475,3,68,34,0,475,476,5,
  	4,0,0,476,477,3,52,26,19,477,484,1,0,0,0,478,479,7,4,0,0,479,484,3,52,
  	26,15,480,481,7,5,0,0,481,484,3,52,26,14,482,484,3,54,27,0,483,472,1,
  	0,0,0,483,478,1,0,0,0,483,480,1,0,0,0,483,482,1,0,0,0,484,536,1,0,0,0,
  	485,486,10,16,0,0,486,487,5,79,0,0,487,535,3,52,26,16,488,489,10,12,0,
  	0,489,490,7,6,0,0,490,535,3,52,26,13,491,492,10,11,0,0,492,493,7,7,0,
  	0,493,535,3,52,26,12,494,495,10,10,0,0,495,496,7,8,0,0,496,535,3,52,26,
  	11,497,498,10,9,0,0,498,499,7,9,0,0,499,535,3,52,26,10,500,501,10,8,0,
  	0,501,502,7,10,0,0,502,535,3,52,26,9,503,504,10,7,0,0,504,505,5,81,0,
  	0,505,535,3,52,26,8,506,507,10,6,0,0,507,508,5,83,0,0,508,535,3,52,26,
  	7,509,510,10,5,0,0,510,511,5,82,0,0,511,535,3,52,26,6,512,513,10,4,0,
  	0,513,514,5,77,0,0,514,535,3,52,26,5,515,516,10,3,0,0,516,517,5,78,0,
  	0,517,535,3,52,26,4,518,519,10,2,0,0,519,520,7,11,0,0,520,535,3,52,26,
  	2,521,522,10,18,0,0,522,523,5,13,0,0,523,524,3,52,26,0,524,525,5,14,0,
  	0,525,535,1,0,0,0,526,527,10,17,0,0,527,535,7,4,0,0,528,529,10,13,0,0,
  	529,532,5,21,0,0,530,533,3,2,1,0,531,533,3,68,34,0,532,530,1,0,0,0,532,
  	531,1,0,0,0,533,535,1,0,0,0,534,485,1,0,0,0,534,488,1,0,0,0,534,491,1,
  	0,0,0,534,494,1,0,0,0,534,497,1,0,0,0,534,500,1,0,0,0,534,503,1,0,0,0,
  	534,506,1,0,0,0,534,509,1,0,0,0,534,512,1,0,0,0,534,515,1,0,0,0,534,518,
  	1,0,0,0,534,521,1,0,0,0,534,526,1,0,0,0,534,528,1,0,0,0,535,538,1,0,0,
  	0,536,534,1,0,0,0,536,537,1,0,0,0,537,53,1,0,0,0,538,536,1,0,0,0,539,
  	540,6,27,-1,0,540,541,5,3,0,0,541,542,3,52,26,0,542,543,5,4,0,0,543,595,
  	1,0,0,0,544,546,5,13,0,0,545,547,3,60,30,0,546,545,1,0,0,0,546,547,1,
  	0,0,0,547,548,1,0,0,0,548,595,5,14,0,0,549,551,5,6,0,0,550,552,3,62,31,
  	0,551,550,1,0,0,0,551,552,1,0,0,0,552,553,1,0,0,0,553,595,5,7,0,0,554,
  	556,5,6,0,0,555,557,3,60,30,0,556,555,1,0,0,0,556,557,1,0,0,0,557,558,
  	1,0,0,0,558,595,5,7,0,0,559,595,5,96,0,0,560,595,5,97,0,0,561,595,5,98,
  	0,0,562,595,5,43,0,0,563,595,5,44,0,0,564,595,5,45,0,0,565,595,5,95,0,
  	0,566,567,5,22,0,0,567,569,5,3,0,0,568,570,3,60,30,0,569,568,1,0,0,0,
  	569,570,1,0,0,0,570,571,1,0,0,0,571,595,5,4,0,0,572,595,3,58,29,0,573,
  	574,5,42,0,0,574,575,3,2,1,0,575,577,5,3,0,0,576,578,3,60,30,0,577,576,
  	1,0,0,0,577,578,1,0,0,0,578,579,1,0,0,0,579,581,5,4,0,0,580,582,3,56,
  	28,0,581,580,1,0,0,0,581,582,1,0,0,0,582,595,1,0,0,0,583,584,5,31,0,0,
  	584,586,5,3,0,0,585,587,3,46,23,0,586,585,1,0,0,0,586,587,1,0,0,0,587,
  	588,1,0,0,0,588,591,5,4,0,0,589,590,5,84,0,0,590,592,3,68,34,0,591,589,
  	1,0,0,0,591,592,1,0,0,0,592,593,1,0,0,0,593,595,3,50,25,0,594,539,1,0,
  	0,0,594,544,1,0,0,0,594,549,1,0,0,0,594,554,1,0,0,0,594,559,1,0,0,0,594,
  	560,1,0,0,0,594,561,1,0,0,0,594,562,1,0,0,0,594,563,1,0,0,0,594,564,1,
  	0,0,0,594,565,1,0,0,0,594,566,1,0,0,0,594,572,1,0,0,0,594,573,1,0,0,0,
  	594,583,1,0,0,0,595,607,1,0,0,0,596,597,10,4,0,0,597,599,5,3,0,0,598,
  	600,3,60,30,0,599,598,1,0,0,0,599,600,1,0,0,0,600,601,1,0,0,0,601,606,
  	5,4,0,0,602,603,10,3,0,0,603,604,5,1,0,0,604,606,5,95,0,0,605,596,1,0,
  	0,0,605,602,1,0,0,0,606,609,1,0,0,0,607,605,1,0,0,0,607,608,1,0,0,0,608,
  	55,1,0,0,0,609,607,1,0,0,0,610,614,5,6,0,0,611,613,3,20,10,0,612,611,
  	1,0,0,0,613,616,1,0,0,0,614,612,1,0,0,0,614,615,1,0,0,0,615,617,1,0,0,
  	0,616,614,1,0,0,0,617,618,5,7,0,0,618,57,1,0,0,0,619,620,5,25,0,0,620,
  	622,5,3,0,0,621,623,3,60,30,0,622,621,1,0,0,0,622,623,1,0,0,0,623,624,
  	1,0,0,0,624,625,5,4,0,0,625,59,1,0,0,0,626,631,3,52,26,0,627,628,5,9,
  	0,0,628,630,3,52,26,0,629,627,1,0,0,0,630,633,1,0,0,0,631,629,1,0,0,0,
  	631,632,1,0,0,0,632,61,1,0,0,0,633,631,1,0,0,0,634,639,3,64,32,0,635,
  	636,5,9,0,0,636,638,3,64,32,0,637,635,1,0,0,0,638,641,1,0,0,0,639,637,
  	1,0,0,0,639,640,1,0,0,0,640,643,1,0,0,0,641,639,1,0,0,0,642,644,5,9,0,
  	0,643,642,1,0,0,0,643,644,1,0,0,0,644,63,1,0,0,0,645,646,3,66,33,0,646,
  	647,5,5,0,0,647,648,3,52,26,0,648,65,1,0,0,0,649,657,5,98,0,0,650,657,
  	5,95,0,0,651,657,5,96,0,0,652,653,5,3,0,0,653,654,3,52,26,0,654,655,5,
  	4,0,0,655,657,1,0,0,0,656,649,1,0,0,0,656,650,1,0,0,0,656,651,1,0,0,0,
  	656,652,1,0,0,0,657,67,1,0,0,0,658,659,6,34,-1,0,659,668,5,49,0,0,660,
  	668,5,50,0,0,661,668,5,51,0,0,662,668,5,52,0,0,663,668,5,53,0,0,664,668,
  	5,54,0,0,665,668,5,55,0,0,666,668,3,2,1,0,667,658,1,0,0,0,667,660,1,0,
  	0,0,667,661,1,0,0,0,667,662,1,0,0,0,667,663,1,0,0,0,667,664,1,0,0,0,667,
  	665,1,0,0,0,667,666,1,0,0,0,668,674,1,0,0,0,669,670,10,1,0,0,670,671,
  	5,13,0,0,671,673,5,14,0,0,672,669,1,0,0,0,673,676,1,0,0,0,674,672,1,0,
  	0,0,674,675,1,0,0,0,675,69,1,0,0,0,676,674,1,0,0,0,80,73,83,98,108,119,
  	123,127,151,155,175,183,191,199,207,216,220,226,233,239,245,250,254,257,
  	261,268,277,285,296,301,309,314,321,326,332,336,342,345,350,356,361,366,
  	373,378,382,392,397,408,414,421,423,427,431,444,459,461,467,483,532,534,
  	536,546,551,556,569,577,581,586,591,594,599,605,607,614,622,631,639,643,
  	656,667,674
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  tzdlangParserStaticData = staticData.release();
}

}

TzdLangParser::TzdLangParser(TokenStream *input) : TzdLangParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

TzdLangParser::TzdLangParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  TzdLangParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *tzdlangParserStaticData->atn, tzdlangParserStaticData->decisionToDFA, tzdlangParserStaticData->sharedContextCache, options);
}

TzdLangParser::~TzdLangParser() {
  delete _interpreter;
}

const atn::ATN& TzdLangParser::getATN() const {
  return *tzdlangParserStaticData->atn;
}

std::string TzdLangParser::getGrammarFileName() const {
  return "TzdLang.g4";
}

const std::vector<std::string>& TzdLangParser::getRuleNames() const {
  return tzdlangParserStaticData->ruleNames;
}

const dfa::Vocabulary& TzdLangParser::getVocabulary() const {
  return tzdlangParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView TzdLangParser::getSerializedATN() const {
  return tzdlangParserStaticData->serializedATN;
}


//----------------- ProgramContext ------------------------------------------------------------------

TzdLangParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::ProgramContext::EOF() {
  return getToken(TzdLangParser::EOF, 0);
}

std::vector<TzdLangParser::StatementContext *> TzdLangParser::ProgramContext::statement() {
  return getRuleContexts<TzdLangParser::StatementContext>();
}

TzdLangParser::StatementContext* TzdLangParser::ProgramContext::statement(size_t i) {
  return getRuleContext<TzdLangParser::StatementContext>(i);
}


size_t TzdLangParser::ProgramContext::getRuleIndex() const {
  return TzdLangParser::RuleProgram;
}


std::any TzdLangParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ProgramContext* TzdLangParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, TzdLangParser::RuleProgram);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(73);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 576175959595589964) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 80)) & 492641) != 0)) {
      setState(70);
      statement();
      setState(75);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(76);
    match(TzdLangParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QualifiedNameContext ------------------------------------------------------------------

TzdLangParser::QualifiedNameContext::QualifiedNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> TzdLangParser::QualifiedNameContext::IDENTIFIER() {
  return getTokens(TzdLangParser::IDENTIFIER);
}

tree::TerminalNode* TzdLangParser::QualifiedNameContext::IDENTIFIER(size_t i) {
  return getToken(TzdLangParser::IDENTIFIER, i);
}


size_t TzdLangParser::QualifiedNameContext::getRuleIndex() const {
  return TzdLangParser::RuleQualifiedName;
}


std::any TzdLangParser::QualifiedNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitQualifiedName(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::QualifiedNameContext* TzdLangParser::qualifiedName() {
  QualifiedNameContext *_localctx = _tracker.createInstance<QualifiedNameContext>(_ctx, getState());
  enterRule(_localctx, 2, TzdLangParser::RuleQualifiedName);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(78);
    match(TzdLangParser::IDENTIFIER);
    setState(83);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(79);
        match(TzdLangParser::T__0);
        setState(80);
        match(TzdLangParser::IDENTIFIER); 
      }
      setState(85);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementContext ------------------------------------------------------------------

TzdLangParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t TzdLangParser::StatementContext::getRuleIndex() const {
  return TzdLangParser::RuleStatement;
}

void TzdLangParser::StatementContext::copyFrom(StatementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SwitchStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::SwitchStmtContext::KW_SWITCH() {
  return getToken(TzdLangParser::KW_SWITCH, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::SwitchStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

std::vector<TzdLangParser::SwitchCaseContext *> TzdLangParser::SwitchStmtContext::switchCase() {
  return getRuleContexts<TzdLangParser::SwitchCaseContext>();
}

TzdLangParser::SwitchCaseContext* TzdLangParser::SwitchStmtContext::switchCase(size_t i) {
  return getRuleContext<TzdLangParser::SwitchCaseContext>(i);
}

TzdLangParser::SwitchDefaultContext* TzdLangParser::SwitchStmtContext::switchDefault() {
  return getRuleContext<TzdLangParser::SwitchDefaultContext>(0);
}

TzdLangParser::SwitchStmtContext::SwitchStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::SwitchStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitSwitchStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ClassDeclStmtContext ------------------------------------------------------------------

TzdLangParser::ClassDeclarationContext* TzdLangParser::ClassDeclStmtContext::classDeclaration() {
  return getRuleContext<TzdLangParser::ClassDeclarationContext>(0);
}

TzdLangParser::ClassDeclStmtContext::ClassDeclStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ClassDeclStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitClassDeclStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BlockStmtContext ------------------------------------------------------------------

TzdLangParser::BlockContext* TzdLangParser::BlockStmtContext::block() {
  return getRuleContext<TzdLangParser::BlockContext>(0);
}

TzdLangParser::BlockStmtContext::BlockStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BlockStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBlockStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ForEachStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::ForEachStmtContext::KW_FOR() {
  return getToken(TzdLangParser::KW_FOR, 0);
}

TzdLangParser::ForEachInitContext* TzdLangParser::ForEachStmtContext::forEachInit() {
  return getRuleContext<TzdLangParser::ForEachInitContext>(0);
}

TzdLangParser::ExpressionContext* TzdLangParser::ForEachStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::StatementContext* TzdLangParser::ForEachStmtContext::statement() {
  return getRuleContext<TzdLangParser::StatementContext>(0);
}

TzdLangParser::ForEachStmtContext::ForEachStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ForEachStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitForEachStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NativeFunDeclStmtContext ------------------------------------------------------------------

TzdLangParser::NativeFunctionDeclarationContext* TzdLangParser::NativeFunDeclStmtContext::nativeFunctionDeclaration() {
  return getRuleContext<TzdLangParser::NativeFunctionDeclarationContext>(0);
}

TzdLangParser::NativeFunDeclStmtContext::NativeFunDeclStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::NativeFunDeclStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNativeFunDeclStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ContinueStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::ContinueStmtContext::KW_CONTINUE() {
  return getToken(TzdLangParser::KW_CONTINUE, 0);
}

TzdLangParser::ContinueStmtContext::ContinueStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ContinueStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitContinueStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ImportStmtContext ------------------------------------------------------------------

TzdLangParser::ImportStatementContext* TzdLangParser::ImportStmtContext::importStatement() {
  return getRuleContext<TzdLangParser::ImportStatementContext>(0);
}

TzdLangParser::ImportStmtContext::ImportStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ImportStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitImportStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IfStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::IfStmtContext::KW_IF() {
  return getToken(TzdLangParser::KW_IF, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::IfStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

std::vector<TzdLangParser::StatementContext *> TzdLangParser::IfStmtContext::statement() {
  return getRuleContexts<TzdLangParser::StatementContext>();
}

TzdLangParser::StatementContext* TzdLangParser::IfStmtContext::statement(size_t i) {
  return getRuleContext<TzdLangParser::StatementContext>(i);
}

tree::TerminalNode* TzdLangParser::IfStmtContext::KW_ELSE() {
  return getToken(TzdLangParser::KW_ELSE, 0);
}

TzdLangParser::IfStmtContext::IfStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::IfStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitIfStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExprStmtContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::ExprStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::ExprStmtContext::ExprStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ExprStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitExprStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- WhileStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::WhileStmtContext::KW_WHILE() {
  return getToken(TzdLangParser::KW_WHILE, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::WhileStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::StatementContext* TzdLangParser::WhileStmtContext::statement() {
  return getRuleContext<TzdLangParser::StatementContext>(0);
}

TzdLangParser::WhileStmtContext::WhileStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::WhileStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitWhileStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AnnotationDeclStmtContext ------------------------------------------------------------------

TzdLangParser::AnnotationDeclarationContext* TzdLangParser::AnnotationDeclStmtContext::annotationDeclaration() {
  return getRuleContext<TzdLangParser::AnnotationDeclarationContext>(0);
}

TzdLangParser::AnnotationDeclStmtContext::AnnotationDeclStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::AnnotationDeclStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAnnotationDeclStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- VarDeclStmtContext ------------------------------------------------------------------

TzdLangParser::VariableDeclarationContext* TzdLangParser::VarDeclStmtContext::variableDeclaration() {
  return getRuleContext<TzdLangParser::VariableDeclarationContext>(0);
}

TzdLangParser::VarDeclStmtContext::VarDeclStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::VarDeclStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitVarDeclStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BreakStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::BreakStmtContext::KW_BREAK() {
  return getToken(TzdLangParser::KW_BREAK, 0);
}

TzdLangParser::BreakStmtContext::BreakStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BreakStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBreakStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- EnumDeclStmtContext ------------------------------------------------------------------

TzdLangParser::EnumDeclarationContext* TzdLangParser::EnumDeclStmtContext::enumDeclaration() {
  return getRuleContext<TzdLangParser::EnumDeclarationContext>(0);
}

TzdLangParser::EnumDeclStmtContext::EnumDeclStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::EnumDeclStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitEnumDeclStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- EmptyStmtContext ------------------------------------------------------------------

TzdLangParser::EmptyStmtContext::EmptyStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::EmptyStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitEmptyStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ReturnStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::ReturnStmtContext::KW_RET() {
  return getToken(TzdLangParser::KW_RET, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::ReturnStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::ReturnStmtContext::ReturnStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ReturnStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitReturnStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ForStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::ForStmtContext::KW_FOR() {
  return getToken(TzdLangParser::KW_FOR, 0);
}

TzdLangParser::StatementContext* TzdLangParser::ForStmtContext::statement() {
  return getRuleContext<TzdLangParser::StatementContext>(0);
}

TzdLangParser::ForInitContext* TzdLangParser::ForStmtContext::forInit() {
  return getRuleContext<TzdLangParser::ForInitContext>(0);
}

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::ForStmtContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::ForStmtContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

TzdLangParser::ForStmtContext::ForStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ForStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitForStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ThrowStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::ThrowStmtContext::KW_THROW() {
  return getToken(TzdLangParser::KW_THROW, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::ThrowStmtContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::ThrowStmtContext::ThrowStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ThrowStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitThrowStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FunDeclStmtContext ------------------------------------------------------------------

TzdLangParser::FunctionDeclarationContext* TzdLangParser::FunDeclStmtContext::functionDeclaration() {
  return getRuleContext<TzdLangParser::FunctionDeclarationContext>(0);
}

TzdLangParser::FunDeclStmtContext::FunDeclStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::FunDeclStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitFunDeclStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- TryCatchStmtContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::TryCatchStmtContext::KW_TRY() {
  return getToken(TzdLangParser::KW_TRY, 0);
}

std::vector<TzdLangParser::BlockContext *> TzdLangParser::TryCatchStmtContext::block() {
  return getRuleContexts<TzdLangParser::BlockContext>();
}

TzdLangParser::BlockContext* TzdLangParser::TryCatchStmtContext::block(size_t i) {
  return getRuleContext<TzdLangParser::BlockContext>(i);
}

tree::TerminalNode* TzdLangParser::TryCatchStmtContext::KW_CATCH() {
  return getToken(TzdLangParser::KW_CATCH, 0);
}

tree::TerminalNode* TzdLangParser::TryCatchStmtContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::TryCatchStmtContext::TryCatchStmtContext(StatementContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::TryCatchStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitTryCatchStmt(this);
  else
    return visitor->visitChildren(this);
}
TzdLangParser::StatementContext* TzdLangParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 4, TzdLangParser::RuleStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(175);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<TzdLangParser::BlockStmtContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(86);
      block();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<TzdLangParser::ClassDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(87);
      classDeclaration();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<TzdLangParser::AnnotationDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(88);
      annotationDeclaration();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<TzdLangParser::EnumDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(89);
      enumDeclaration();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<TzdLangParser::FunDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(90);
      functionDeclaration();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<TzdLangParser::NativeFunDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(91);
      nativeFunctionDeclaration();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<TzdLangParser::VarDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(92);
      variableDeclaration();
      setState(93);
      match(TzdLangParser::T__1);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<TzdLangParser::ImportStmtContext>(_localctx);
      enterOuterAlt(_localctx, 8);
      setState(95);
      importStatement();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<TzdLangParser::ReturnStmtContext>(_localctx);
      enterOuterAlt(_localctx, 9);
      setState(96);
      match(TzdLangParser::KW_RET);
      setState(98);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(97);
        expression(0);
      }
      setState(100);
      match(TzdLangParser::T__1);
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<TzdLangParser::IfStmtContext>(_localctx);
      enterOuterAlt(_localctx, 10);
      setState(101);
      match(TzdLangParser::KW_IF);
      setState(102);
      match(TzdLangParser::T__2);
      setState(103);
      expression(0);
      setState(104);
      match(TzdLangParser::T__3);
      setState(105);
      statement();
      setState(108);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
      case 1: {
        setState(106);
        match(TzdLangParser::KW_ELSE);
        setState(107);
        statement();
        break;
      }

      default:
        break;
      }
      break;
    }

    case 11: {
      _localctx = _tracker.createInstance<TzdLangParser::WhileStmtContext>(_localctx);
      enterOuterAlt(_localctx, 11);
      setState(110);
      match(TzdLangParser::KW_WHILE);
      setState(111);
      match(TzdLangParser::T__2);
      setState(112);
      expression(0);
      setState(113);
      match(TzdLangParser::T__3);
      setState(114);
      statement();
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<TzdLangParser::ForStmtContext>(_localctx);
      enterOuterAlt(_localctx, 12);
      setState(116);
      match(TzdLangParser::KW_FOR);
      setState(117);
      match(TzdLangParser::T__2);
      setState(119);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 575963775232942152) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(118);
        forInit();
      }
      setState(121);
      match(TzdLangParser::T__1);
      setState(123);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(122);
        antlrcpp::downCast<ForStmtContext *>(_localctx)->cond = expression(0);
      }
      setState(125);
      match(TzdLangParser::T__1);
      setState(127);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(126);
        antlrcpp::downCast<ForStmtContext *>(_localctx)->step = expression(0);
      }
      setState(129);
      match(TzdLangParser::T__3);
      setState(130);
      statement();
      break;
    }

    case 13: {
      _localctx = _tracker.createInstance<TzdLangParser::ForEachStmtContext>(_localctx);
      enterOuterAlt(_localctx, 13);
      setState(131);
      match(TzdLangParser::KW_FOR);
      setState(132);
      match(TzdLangParser::T__2);
      setState(133);
      forEachInit();
      setState(134);
      match(TzdLangParser::T__4);
      setState(135);
      expression(0);
      setState(136);
      match(TzdLangParser::T__3);
      setState(137);
      statement();
      break;
    }

    case 14: {
      _localctx = _tracker.createInstance<TzdLangParser::BreakStmtContext>(_localctx);
      enterOuterAlt(_localctx, 14);
      setState(139);
      match(TzdLangParser::KW_BREAK);
      setState(140);
      match(TzdLangParser::T__1);
      break;
    }

    case 15: {
      _localctx = _tracker.createInstance<TzdLangParser::ContinueStmtContext>(_localctx);
      enterOuterAlt(_localctx, 15);
      setState(141);
      match(TzdLangParser::KW_CONTINUE);
      setState(142);
      match(TzdLangParser::T__1);
      break;
    }

    case 16: {
      _localctx = _tracker.createInstance<TzdLangParser::SwitchStmtContext>(_localctx);
      enterOuterAlt(_localctx, 16);
      setState(143);
      match(TzdLangParser::KW_SWITCH);
      setState(144);
      match(TzdLangParser::T__2);
      setState(145);
      expression(0);
      setState(146);
      match(TzdLangParser::T__3);
      setState(147);
      match(TzdLangParser::T__5);
      setState(151);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == TzdLangParser::KW_CASE) {
        setState(148);
        switchCase();
        setState(153);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(155);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::KW_DEFAULT) {
        setState(154);
        switchDefault();
      }
      setState(157);
      match(TzdLangParser::T__6);
      break;
    }

    case 17: {
      _localctx = _tracker.createInstance<TzdLangParser::TryCatchStmtContext>(_localctx);
      enterOuterAlt(_localctx, 17);
      setState(159);
      match(TzdLangParser::KW_TRY);
      setState(160);
      block();
      setState(161);
      match(TzdLangParser::KW_CATCH);
      setState(162);
      match(TzdLangParser::T__2);
      setState(163);
      match(TzdLangParser::IDENTIFIER);
      setState(164);
      match(TzdLangParser::T__3);
      setState(165);
      block();
      break;
    }

    case 18: {
      _localctx = _tracker.createInstance<TzdLangParser::ThrowStmtContext>(_localctx);
      enterOuterAlt(_localctx, 18);
      setState(167);
      match(TzdLangParser::KW_THROW);
      setState(168);
      expression(0);
      setState(169);
      match(TzdLangParser::T__1);
      break;
    }

    case 19: {
      _localctx = _tracker.createInstance<TzdLangParser::ExprStmtContext>(_localctx);
      enterOuterAlt(_localctx, 19);
      setState(171);
      expression(0);
      setState(172);
      match(TzdLangParser::T__1);
      break;
    }

    case 20: {
      _localctx = _tracker.createInstance<TzdLangParser::EmptyStmtContext>(_localctx);
      enterOuterAlt(_localctx, 20);
      setState(174);
      match(TzdLangParser::T__1);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SwitchCaseContext ------------------------------------------------------------------

TzdLangParser::SwitchCaseContext::SwitchCaseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::SwitchCaseContext::KW_CASE() {
  return getToken(TzdLangParser::KW_CASE, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::SwitchCaseContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

std::vector<TzdLangParser::StatementContext *> TzdLangParser::SwitchCaseContext::statement() {
  return getRuleContexts<TzdLangParser::StatementContext>();
}

TzdLangParser::StatementContext* TzdLangParser::SwitchCaseContext::statement(size_t i) {
  return getRuleContext<TzdLangParser::StatementContext>(i);
}


size_t TzdLangParser::SwitchCaseContext::getRuleIndex() const {
  return TzdLangParser::RuleSwitchCase;
}


std::any TzdLangParser::SwitchCaseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitSwitchCase(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::SwitchCaseContext* TzdLangParser::switchCase() {
  SwitchCaseContext *_localctx = _tracker.createInstance<SwitchCaseContext>(_ctx, getState());
  enterRule(_localctx, 6, TzdLangParser::RuleSwitchCase);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(177);
    match(TzdLangParser::KW_CASE);
    setState(178);
    expression(0);
    setState(179);
    match(TzdLangParser::T__4);
    setState(183);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 576175959595589964) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 80)) & 492641) != 0)) {
      setState(180);
      statement();
      setState(185);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SwitchDefaultContext ------------------------------------------------------------------

TzdLangParser::SwitchDefaultContext::SwitchDefaultContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::SwitchDefaultContext::KW_DEFAULT() {
  return getToken(TzdLangParser::KW_DEFAULT, 0);
}

std::vector<TzdLangParser::StatementContext *> TzdLangParser::SwitchDefaultContext::statement() {
  return getRuleContexts<TzdLangParser::StatementContext>();
}

TzdLangParser::StatementContext* TzdLangParser::SwitchDefaultContext::statement(size_t i) {
  return getRuleContext<TzdLangParser::StatementContext>(i);
}


size_t TzdLangParser::SwitchDefaultContext::getRuleIndex() const {
  return TzdLangParser::RuleSwitchDefault;
}


std::any TzdLangParser::SwitchDefaultContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitSwitchDefault(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::SwitchDefaultContext* TzdLangParser::switchDefault() {
  SwitchDefaultContext *_localctx = _tracker.createInstance<SwitchDefaultContext>(_ctx, getState());
  enterRule(_localctx, 8, TzdLangParser::RuleSwitchDefault);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(186);
    match(TzdLangParser::KW_DEFAULT);
    setState(187);
    match(TzdLangParser::T__4);
    setState(191);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 576175959595589964) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 80)) & 492641) != 0)) {
      setState(188);
      statement();
      setState(193);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnnotationDeclarationContext ------------------------------------------------------------------

TzdLangParser::AnnotationDeclarationContext::AnnotationDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::AnnotationDeclarationContext::KW_CLASS() {
  return getToken(TzdLangParser::KW_CLASS, 0);
}

tree::TerminalNode* TzdLangParser::AnnotationDeclarationContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::ParamListContext* TzdLangParser::AnnotationDeclarationContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}


size_t TzdLangParser::AnnotationDeclarationContext::getRuleIndex() const {
  return TzdLangParser::RuleAnnotationDeclaration;
}


std::any TzdLangParser::AnnotationDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAnnotationDeclaration(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::AnnotationDeclarationContext* TzdLangParser::annotationDeclaration() {
  AnnotationDeclarationContext *_localctx = _tracker.createInstance<AnnotationDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 10, TzdLangParser::RuleAnnotationDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(194);
    match(TzdLangParser::KW_CLASS);
    setState(195);
    match(TzdLangParser::T__7);
    setState(196);
    match(TzdLangParser::IDENTIFIER);
    setState(197);
    match(TzdLangParser::T__2);
    setState(199);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 32) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
      setState(198);
      paramList();
    }
    setState(201);
    match(TzdLangParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EnumDeclarationContext ------------------------------------------------------------------

TzdLangParser::EnumDeclarationContext::EnumDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::EnumDeclarationContext::KW_ENUM() {
  return getToken(TzdLangParser::KW_ENUM, 0);
}

tree::TerminalNode* TzdLangParser::EnumDeclarationContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::EnumListContext* TzdLangParser::EnumDeclarationContext::enumList() {
  return getRuleContext<TzdLangParser::EnumListContext>(0);
}


size_t TzdLangParser::EnumDeclarationContext::getRuleIndex() const {
  return TzdLangParser::RuleEnumDeclaration;
}


std::any TzdLangParser::EnumDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitEnumDeclaration(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::EnumDeclarationContext* TzdLangParser::enumDeclaration() {
  EnumDeclarationContext *_localctx = _tracker.createInstance<EnumDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 12, TzdLangParser::RuleEnumDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(203);
    match(TzdLangParser::KW_ENUM);
    setState(204);
    match(TzdLangParser::IDENTIFIER);
    setState(205);
    match(TzdLangParser::T__5);
    setState(207);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::IDENTIFIER) {
      setState(206);
      enumList();
    }
    setState(209);
    match(TzdLangParser::T__6);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EnumListContext ------------------------------------------------------------------

TzdLangParser::EnumListContext::EnumListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> TzdLangParser::EnumListContext::IDENTIFIER() {
  return getTokens(TzdLangParser::IDENTIFIER);
}

tree::TerminalNode* TzdLangParser::EnumListContext::IDENTIFIER(size_t i) {
  return getToken(TzdLangParser::IDENTIFIER, i);
}


size_t TzdLangParser::EnumListContext::getRuleIndex() const {
  return TzdLangParser::RuleEnumList;
}


std::any TzdLangParser::EnumListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitEnumList(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::EnumListContext* TzdLangParser::enumList() {
  EnumListContext *_localctx = _tracker.createInstance<EnumListContext>(_ctx, getState());
  enterRule(_localctx, 14, TzdLangParser::RuleEnumList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(211);
    match(TzdLangParser::IDENTIFIER);
    setState(216);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(212);
      match(TzdLangParser::T__8);
      setState(213);
      match(TzdLangParser::IDENTIFIER);
      setState(218);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassDeclarationContext ------------------------------------------------------------------

TzdLangParser::ClassDeclarationContext::ClassDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::ClassDeclarationContext::KW_CLASS() {
  return getToken(TzdLangParser::KW_CLASS, 0);
}

std::vector<TzdLangParser::QualifiedNameContext *> TzdLangParser::ClassDeclarationContext::qualifiedName() {
  return getRuleContexts<TzdLangParser::QualifiedNameContext>();
}

TzdLangParser::QualifiedNameContext* TzdLangParser::ClassDeclarationContext::qualifiedName(size_t i) {
  return getRuleContext<TzdLangParser::QualifiedNameContext>(i);
}

TzdLangParser::ClassBodyContext* TzdLangParser::ClassDeclarationContext::classBody() {
  return getRuleContext<TzdLangParser::ClassBodyContext>(0);
}

TzdLangParser::AnnotationUsageContext* TzdLangParser::ClassDeclarationContext::annotationUsage() {
  return getRuleContext<TzdLangParser::AnnotationUsageContext>(0);
}

tree::TerminalNode* TzdLangParser::ClassDeclarationContext::KW_EXTENDS() {
  return getToken(TzdLangParser::KW_EXTENDS, 0);
}


size_t TzdLangParser::ClassDeclarationContext::getRuleIndex() const {
  return TzdLangParser::RuleClassDeclaration;
}


std::any TzdLangParser::ClassDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitClassDeclaration(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ClassDeclarationContext* TzdLangParser::classDeclaration() {
  ClassDeclarationContext *_localctx = _tracker.createInstance<ClassDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 16, TzdLangParser::RuleClassDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(245);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(220);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(219);
        annotationUsage();
      }
      setState(222);
      match(TzdLangParser::KW_CLASS);
      setState(223);
      qualifiedName();
      setState(226);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__4) {
        setState(224);
        match(TzdLangParser::T__4);
        setState(225);
        qualifiedName();
      }
      setState(228);
      match(TzdLangParser::T__5);
      setState(229);
      classBody();
      setState(230);
      match(TzdLangParser::T__6);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(233);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(232);
        annotationUsage();
      }
      setState(235);
      match(TzdLangParser::KW_CLASS);
      setState(236);
      qualifiedName();
      setState(239);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::KW_EXTENDS) {
        setState(237);
        match(TzdLangParser::KW_EXTENDS);
        setState(238);
        qualifiedName();
      }
      setState(241);
      match(TzdLangParser::T__5);
      setState(242);
      classBody();
      setState(243);
      match(TzdLangParser::T__6);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassBodyContext ------------------------------------------------------------------

TzdLangParser::ClassBodyContext::ClassBodyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::ClassMemberContext *> TzdLangParser::ClassBodyContext::classMember() {
  return getRuleContexts<TzdLangParser::ClassMemberContext>();
}

TzdLangParser::ClassMemberContext* TzdLangParser::ClassBodyContext::classMember(size_t i) {
  return getRuleContext<TzdLangParser::ClassMemberContext>(i);
}


size_t TzdLangParser::ClassBodyContext::getRuleIndex() const {
  return TzdLangParser::RuleClassBody;
}


std::any TzdLangParser::ClassBodyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitClassBody(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ClassBodyContext* TzdLangParser::classBody() {
  ClassBodyContext *_localctx = _tracker.createInstance<ClassBodyContext>(_ctx, getState());
  enterRule(_localctx, 18, TzdLangParser::RuleClassBody);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(250);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 4035936512) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(247);
      classMember();
      setState(252);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassMemberContext ------------------------------------------------------------------

TzdLangParser::ClassMemberContext::ClassMemberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

TzdLangParser::MemberDeclContext* TzdLangParser::ClassMemberContext::memberDecl() {
  return getRuleContext<TzdLangParser::MemberDeclContext>(0);
}

TzdLangParser::AnnotationUsageContext* TzdLangParser::ClassMemberContext::annotationUsage() {
  return getRuleContext<TzdLangParser::AnnotationUsageContext>(0);
}

TzdLangParser::AccessModifierContext* TzdLangParser::ClassMemberContext::accessModifier() {
  return getRuleContext<TzdLangParser::AccessModifierContext>(0);
}

TzdLangParser::NativeFunctionDeclarationContext* TzdLangParser::ClassMemberContext::nativeFunctionDeclaration() {
  return getRuleContext<TzdLangParser::NativeFunctionDeclarationContext>(0);
}


size_t TzdLangParser::ClassMemberContext::getRuleIndex() const {
  return TzdLangParser::RuleClassMember;
}


std::any TzdLangParser::ClassMemberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitClassMember(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ClassMemberContext* TzdLangParser::classMember() {
  ClassMemberContext *_localctx = _tracker.createInstance<ClassMemberContext>(_ctx, getState());
  enterRule(_localctx, 20, TzdLangParser::RuleClassMember);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(261);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(254);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(253);
        annotationUsage();
      }
      setState(257);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1879048192) != 0)) {
        setState(256);
        accessModifier();
      }
      setState(259);
      memberDecl();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(260);
      nativeFunctionDeclaration();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MemberDeclContext ------------------------------------------------------------------

TzdLangParser::MemberDeclContext::MemberDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t TzdLangParser::MemberDeclContext::getRuleIndex() const {
  return TzdLangParser::RuleMemberDecl;
}

void TzdLangParser::MemberDeclContext::copyFrom(MemberDeclContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ConstructorDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::ConstructorDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::BlockContext* TzdLangParser::ConstructorDeclContext::block() {
  return getRuleContext<TzdLangParser::BlockContext>(0);
}

TzdLangParser::ParamListContext* TzdLangParser::ConstructorDeclContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

TzdLangParser::ConstructorDeclContext::ConstructorDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ConstructorDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitConstructorDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FieldVarDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::FieldVarDeclContext::KW_VAR() {
  return getToken(TzdLangParser::KW_VAR, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::FieldVarDeclContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

tree::TerminalNode* TzdLangParser::FieldVarDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

tree::TerminalNode* TzdLangParser::FieldVarDeclContext::ASSIGN() {
  return getToken(TzdLangParser::ASSIGN, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::FieldVarDeclContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::FieldVarDeclContext::FieldVarDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::FieldVarDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitFieldVarDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MethodStaticDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::MethodStaticDeclContext::KW_STATIC() {
  return getToken(TzdLangParser::KW_STATIC, 0);
}

tree::TerminalNode* TzdLangParser::MethodStaticDeclContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}

tree::TerminalNode* TzdLangParser::MethodStaticDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::BlockContext* TzdLangParser::MethodStaticDeclContext::block() {
  return getRuleContext<TzdLangParser::BlockContext>(0);
}

TzdLangParser::ParamListContext* TzdLangParser::MethodStaticDeclContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

tree::TerminalNode* TzdLangParser::MethodStaticDeclContext::ARROW() {
  return getToken(TzdLangParser::ARROW, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::MethodStaticDeclContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::MethodStaticDeclContext::MethodStaticDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::MethodStaticDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMethodStaticDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MethodDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::MethodDeclContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}

tree::TerminalNode* TzdLangParser::MethodDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::BlockContext* TzdLangParser::MethodDeclContext::block() {
  return getRuleContext<TzdLangParser::BlockContext>(0);
}

TzdLangParser::ParamListContext* TzdLangParser::MethodDeclContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

tree::TerminalNode* TzdLangParser::MethodDeclContext::ARROW() {
  return getToken(TzdLangParser::ARROW, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::MethodDeclContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::MethodDeclContext::MethodDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::MethodDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMethodDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MethodAbstractDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::MethodAbstractDeclContext::KW_ABSTRACT() {
  return getToken(TzdLangParser::KW_ABSTRACT, 0);
}

tree::TerminalNode* TzdLangParser::MethodAbstractDeclContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}

tree::TerminalNode* TzdLangParser::MethodAbstractDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::ParamListContext* TzdLangParser::MethodAbstractDeclContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

tree::TerminalNode* TzdLangParser::MethodAbstractDeclContext::ARROW() {
  return getToken(TzdLangParser::ARROW, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::MethodAbstractDeclContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::MethodAbstractDeclContext::MethodAbstractDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::MethodAbstractDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMethodAbstractDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FieldConstDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::FieldConstDeclContext::KW_CONST() {
  return getToken(TzdLangParser::KW_CONST, 0);
}

tree::TerminalNode* TzdLangParser::FieldConstDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

tree::TerminalNode* TzdLangParser::FieldConstDeclContext::ASSIGN() {
  return getToken(TzdLangParser::ASSIGN, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::FieldConstDeclContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::FieldConstDeclContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::FieldConstDeclContext::FieldConstDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::FieldConstDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitFieldConstDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FieldLetDeclContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::FieldLetDeclContext::KW_LET() {
  return getToken(TzdLangParser::KW_LET, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::FieldLetDeclContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

tree::TerminalNode* TzdLangParser::FieldLetDeclContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

tree::TerminalNode* TzdLangParser::FieldLetDeclContext::ASSIGN() {
  return getToken(TzdLangParser::ASSIGN, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::FieldLetDeclContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::FieldLetDeclContext::FieldLetDeclContext(MemberDeclContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::FieldLetDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitFieldLetDecl(this);
  else
    return visitor->visitChildren(this);
}
TzdLangParser::MemberDeclContext* TzdLangParser::memberDecl() {
  MemberDeclContext *_localctx = _tracker.createInstance<MemberDeclContext>(_ctx, getState());
  enterRule(_localctx, 22, TzdLangParser::RuleMemberDecl);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(336);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::KW_VAR: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldVarDeclContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(263);
        match(TzdLangParser::KW_VAR);
        setState(264);
        typeType(0);
        setState(265);
        match(TzdLangParser::IDENTIFIER);
        setState(268);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ASSIGN) {
          setState(266);
          match(TzdLangParser::ASSIGN);
          setState(267);
          expression(0);
        }
        setState(270);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_LET: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldLetDeclContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(272);
        match(TzdLangParser::KW_LET);
        setState(273);
        typeType(0);
        setState(274);
        match(TzdLangParser::IDENTIFIER);
        setState(277);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ASSIGN) {
          setState(275);
          match(TzdLangParser::ASSIGN);
          setState(276);
          expression(0);
        }
        setState(279);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_CONST: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldConstDeclContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(281);
        match(TzdLangParser::KW_CONST);
        setState(282);
        match(TzdLangParser::IDENTIFIER);
        setState(285);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::T__4) {
          setState(283);
          match(TzdLangParser::T__4);
          setState(284);
          typeType(0);
        }
        setState(287);
        match(TzdLangParser::ASSIGN);
        setState(288);
        expression(0);
        setState(289);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_STATIC: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodStaticDeclContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(291);
        match(TzdLangParser::KW_STATIC);
        setState(292);
        match(TzdLangParser::KW_FUN);
        setState(293);
        match(TzdLangParser::IDENTIFIER);
        setState(294);
        match(TzdLangParser::T__2);
        setState(296);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 32) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
          setState(295);
          paramList();
        }
        setState(298);
        match(TzdLangParser::T__3);
        setState(301);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ARROW) {
          setState(299);
          match(TzdLangParser::ARROW);
          setState(300);
          typeType(0);
        }
        setState(303);
        block();
        break;
      }

      case TzdLangParser::KW_ABSTRACT: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodAbstractDeclContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(304);
        match(TzdLangParser::KW_ABSTRACT);
        setState(305);
        match(TzdLangParser::KW_FUN);
        setState(306);
        match(TzdLangParser::IDENTIFIER);
        setState(307);
        match(TzdLangParser::T__2);
        setState(309);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 32) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
          setState(308);
          paramList();
        }
        setState(311);
        match(TzdLangParser::T__3);
        setState(314);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ARROW) {
          setState(312);
          match(TzdLangParser::ARROW);
          setState(313);
          typeType(0);
        }
        setState(316);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_FUN: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodDeclContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(317);
        match(TzdLangParser::KW_FUN);
        setState(318);
        match(TzdLangParser::IDENTIFIER);
        setState(319);
        match(TzdLangParser::T__2);
        setState(321);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 32) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
          setState(320);
          paramList();
        }
        setState(323);
        match(TzdLangParser::T__3);
        setState(326);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ARROW) {
          setState(324);
          match(TzdLangParser::ARROW);
          setState(325);
          typeType(0);
        }
        setState(328);
        block();
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        _localctx = _tracker.createInstance<TzdLangParser::ConstructorDeclContext>(_localctx);
        enterOuterAlt(_localctx, 7);
        setState(329);
        match(TzdLangParser::IDENTIFIER);
        setState(330);
        match(TzdLangParser::T__2);
        setState(332);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 32) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
          setState(331);
          paramList();
        }
        setState(334);
        match(TzdLangParser::T__3);
        setState(335);
        block();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnnotationUsageContext ------------------------------------------------------------------

TzdLangParser::AnnotationUsageContext::AnnotationUsageContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::AnnotationUsageContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::ExprListContext* TzdLangParser::AnnotationUsageContext::exprList() {
  return getRuleContext<TzdLangParser::ExprListContext>(0);
}


size_t TzdLangParser::AnnotationUsageContext::getRuleIndex() const {
  return TzdLangParser::RuleAnnotationUsage;
}


std::any TzdLangParser::AnnotationUsageContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAnnotationUsage(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::AnnotationUsageContext* TzdLangParser::annotationUsage() {
  AnnotationUsageContext *_localctx = _tracker.createInstance<AnnotationUsageContext>(_ctx, getState());
  enterRule(_localctx, 24, TzdLangParser::RuleAnnotationUsage);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(338);
    match(TzdLangParser::T__7);
    setState(339);
    match(TzdLangParser::IDENTIFIER);
    setState(345);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__2) {
      setState(340);
      match(TzdLangParser::T__2);
      setState(342);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(341);
        exprList();
      }
      setState(344);
      match(TzdLangParser::T__3);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AccessModifierContext ------------------------------------------------------------------

TzdLangParser::AccessModifierContext::AccessModifierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::AccessModifierContext::KW_PUBLIC() {
  return getToken(TzdLangParser::KW_PUBLIC, 0);
}

tree::TerminalNode* TzdLangParser::AccessModifierContext::KW_PRIVATE() {
  return getToken(TzdLangParser::KW_PRIVATE, 0);
}

tree::TerminalNode* TzdLangParser::AccessModifierContext::KW_PROTECTED() {
  return getToken(TzdLangParser::KW_PROTECTED, 0);
}


size_t TzdLangParser::AccessModifierContext::getRuleIndex() const {
  return TzdLangParser::RuleAccessModifier;
}


std::any TzdLangParser::AccessModifierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAccessModifier(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::AccessModifierContext* TzdLangParser::accessModifier() {
  AccessModifierContext *_localctx = _tracker.createInstance<AccessModifierContext>(_ctx, getState());
  enterRule(_localctx, 26, TzdLangParser::RuleAccessModifier);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(347);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1879048192) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FunctionDeclarationContext ------------------------------------------------------------------

TzdLangParser::FunctionDeclarationContext::FunctionDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::FunctionDeclarationContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}

tree::TerminalNode* TzdLangParser::FunctionDeclarationContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::BlockContext* TzdLangParser::FunctionDeclarationContext::block() {
  return getRuleContext<TzdLangParser::BlockContext>(0);
}

TzdLangParser::AnnotationUsageContext* TzdLangParser::FunctionDeclarationContext::annotationUsage() {
  return getRuleContext<TzdLangParser::AnnotationUsageContext>(0);
}

TzdLangParser::ParamListContext* TzdLangParser::FunctionDeclarationContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

tree::TerminalNode* TzdLangParser::FunctionDeclarationContext::ARROW() {
  return getToken(TzdLangParser::ARROW, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::FunctionDeclarationContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}


size_t TzdLangParser::FunctionDeclarationContext::getRuleIndex() const {
  return TzdLangParser::RuleFunctionDeclaration;
}


std::any TzdLangParser::FunctionDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitFunctionDeclaration(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::FunctionDeclarationContext* TzdLangParser::functionDeclaration() {
  FunctionDeclarationContext *_localctx = _tracker.createInstance<FunctionDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 28, TzdLangParser::RuleFunctionDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(350);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__7) {
      setState(349);
      annotationUsage();
    }
    setState(352);
    match(TzdLangParser::KW_FUN);
    setState(353);
    match(TzdLangParser::IDENTIFIER);
    setState(354);
    match(TzdLangParser::T__2);
    setState(356);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 32) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
      setState(355);
      paramList();
    }
    setState(358);
    match(TzdLangParser::T__3);
    setState(361);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::ARROW) {
      setState(359);
      match(TzdLangParser::ARROW);
      setState(360);
      typeType(0);
    }
    setState(363);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NativeFunctionDeclarationContext ------------------------------------------------------------------

TzdLangParser::NativeFunctionDeclarationContext::NativeFunctionDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::NativeFunctionDeclarationContext::KW_NATIVE() {
  return getToken(TzdLangParser::KW_NATIVE, 0);
}

tree::TerminalNode* TzdLangParser::NativeFunctionDeclarationContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}

tree::TerminalNode* TzdLangParser::NativeFunctionDeclarationContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::AnnotationUsageContext* TzdLangParser::NativeFunctionDeclarationContext::annotationUsage() {
  return getRuleContext<TzdLangParser::AnnotationUsageContext>(0);
}

TzdLangParser::ParamListContext* TzdLangParser::NativeFunctionDeclarationContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

tree::TerminalNode* TzdLangParser::NativeFunctionDeclarationContext::ARROW() {
  return getToken(TzdLangParser::ARROW, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::NativeFunctionDeclarationContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::NativeAttrListContext* TzdLangParser::NativeFunctionDeclarationContext::nativeAttrList() {
  return getRuleContext<TzdLangParser::NativeAttrListContext>(0);
}


size_t TzdLangParser::NativeFunctionDeclarationContext::getRuleIndex() const {
  return TzdLangParser::RuleNativeFunctionDeclaration;
}


std::any TzdLangParser::NativeFunctionDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNativeFunctionDeclaration(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::NativeFunctionDeclarationContext* TzdLangParser::nativeFunctionDeclaration() {
  NativeFunctionDeclarationContext *_localctx = _tracker.createInstance<NativeFunctionDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 30, TzdLangParser::RuleNativeFunctionDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(366);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__7) {
      setState(365);
      annotationUsage();
    }
    setState(368);
    match(TzdLangParser::KW_NATIVE);
    setState(369);
    match(TzdLangParser::KW_FUN);
    setState(370);
    match(TzdLangParser::IDENTIFIER);
    setState(371);
    match(TzdLangParser::T__2);
    setState(373);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 32) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
      setState(372);
      paramList();
    }
    setState(375);
    match(TzdLangParser::T__3);
    setState(378);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::ARROW) {
      setState(376);
      match(TzdLangParser::ARROW);
      setState(377);
      typeType(0);
    }
    setState(380);
    match(TzdLangParser::T__2);
    setState(382);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 71494650526964736) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(381);
      nativeAttrList();
    }
    setState(384);
    match(TzdLangParser::T__3);
    setState(385);
    match(TzdLangParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NativeAttrListContext ------------------------------------------------------------------

TzdLangParser::NativeAttrListContext::NativeAttrListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::NativeAttrContext *> TzdLangParser::NativeAttrListContext::nativeAttr() {
  return getRuleContexts<TzdLangParser::NativeAttrContext>();
}

TzdLangParser::NativeAttrContext* TzdLangParser::NativeAttrListContext::nativeAttr(size_t i) {
  return getRuleContext<TzdLangParser::NativeAttrContext>(i);
}


size_t TzdLangParser::NativeAttrListContext::getRuleIndex() const {
  return TzdLangParser::RuleNativeAttrList;
}


std::any TzdLangParser::NativeAttrListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNativeAttrList(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::NativeAttrListContext* TzdLangParser::nativeAttrList() {
  NativeAttrListContext *_localctx = _tracker.createInstance<NativeAttrListContext>(_ctx, getState());
  enterRule(_localctx, 32, TzdLangParser::RuleNativeAttrList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(387);
    nativeAttr();
    setState(392);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(388);
      match(TzdLangParser::T__8);
      setState(389);
      nativeAttr();
      setState(394);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NativeAttrContext ------------------------------------------------------------------

TzdLangParser::NativeAttrContext::NativeAttrContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::NativeAttrContext::ASSIGN() {
  return getToken(TzdLangParser::ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::NativeAttrContext::STRING() {
  return getToken(TzdLangParser::STRING, 0);
}

tree::TerminalNode* TzdLangParser::NativeAttrContext::INTEGER() {
  return getToken(TzdLangParser::INTEGER, 0);
}

tree::TerminalNode* TzdLangParser::NativeAttrContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::NativePropKeyContext* TzdLangParser::NativeAttrContext::nativePropKey() {
  return getRuleContext<TzdLangParser::NativePropKeyContext>(0);
}


size_t TzdLangParser::NativeAttrContext::getRuleIndex() const {
  return TzdLangParser::RuleNativeAttr;
}


std::any TzdLangParser::NativeAttrContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNativeAttr(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::NativeAttrContext* TzdLangParser::nativeAttr() {
  NativeAttrContext *_localctx = _tracker.createInstance<NativeAttrContext>(_ctx, getState());
  enterRule(_localctx, 34, TzdLangParser::RuleNativeAttr);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(397);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::IDENTIFIER: {
        setState(395);
        match(TzdLangParser::IDENTIFIER);
        break;
      }

      case TzdLangParser::T__9:
      case TzdLangParser::T__10:
      case TzdLangParser::T__11:
      case TzdLangParser::KW_FUN:
      case TzdLangParser::KW_RET:
      case TzdLangParser::T_INT:
      case TzdLangParser::T_FLOAT:
      case TzdLangParser::T_STRING:
      case TzdLangParser::T_BOOL:
      case TzdLangParser::T_VOID:
      case TzdLangParser::T_PTR:
      case TzdLangParser::T_FUNCTION: {
        setState(396);
        nativePropKey();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(399);
    match(TzdLangParser::ASSIGN);
    setState(400);
    _la = _input->LA(1);
    if (!(_la == TzdLangParser::INTEGER

    || _la == TzdLangParser::STRING)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NativePropKeyContext ------------------------------------------------------------------

TzdLangParser::NativePropKeyContext::NativePropKeyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_INT() {
  return getToken(TzdLangParser::T_INT, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_STRING() {
  return getToken(TzdLangParser::T_STRING, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_FLOAT() {
  return getToken(TzdLangParser::T_FLOAT, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_BOOL() {
  return getToken(TzdLangParser::T_BOOL, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_VOID() {
  return getToken(TzdLangParser::T_VOID, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_PTR() {
  return getToken(TzdLangParser::T_PTR, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::T_FUNCTION() {
  return getToken(TzdLangParser::T_FUNCTION, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::KW_RET() {
  return getToken(TzdLangParser::KW_RET, 0);
}

tree::TerminalNode* TzdLangParser::NativePropKeyContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}


size_t TzdLangParser::NativePropKeyContext::getRuleIndex() const {
  return TzdLangParser::RuleNativePropKey;
}


std::any TzdLangParser::NativePropKeyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNativePropKey(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::NativePropKeyContext* TzdLangParser::nativePropKey() {
  NativePropKeyContext *_localctx = _tracker.createInstance<NativePropKeyContext>(_ctx, getState());
  enterRule(_localctx, 36, TzdLangParser::RuleNativePropKey);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(402);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 71494650526964736) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VariableDeclarationContext ------------------------------------------------------------------

TzdLangParser::VariableDeclarationContext::VariableDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

TzdLangParser::TypeTypeContext* TzdLangParser::VariableDeclarationContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

tree::TerminalNode* TzdLangParser::VariableDeclarationContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

tree::TerminalNode* TzdLangParser::VariableDeclarationContext::ASSIGN() {
  return getToken(TzdLangParser::ASSIGN, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::VariableDeclarationContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

tree::TerminalNode* TzdLangParser::VariableDeclarationContext::KW_VAR() {
  return getToken(TzdLangParser::KW_VAR, 0);
}


size_t TzdLangParser::VariableDeclarationContext::getRuleIndex() const {
  return TzdLangParser::RuleVariableDeclaration;
}


std::any TzdLangParser::VariableDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitVariableDeclaration(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::VariableDeclarationContext* TzdLangParser::variableDeclaration() {
  VariableDeclarationContext *_localctx = _tracker.createInstance<VariableDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 38, TzdLangParser::RuleVariableDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(423);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 49, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(404);
      typeType(0);
      setState(405);
      match(TzdLangParser::IDENTIFIER);
      setState(408);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(406);
        match(TzdLangParser::ASSIGN);
        setState(407);
        expression(0);
      }
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(410);
      match(TzdLangParser::KW_VAR);
      setState(411);
      match(TzdLangParser::IDENTIFIER);
      setState(414);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(412);
        match(TzdLangParser::ASSIGN);
        setState(413);
        expression(0);
      }
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(416);
      match(TzdLangParser::IDENTIFIER);
      setState(417);
      match(TzdLangParser::T__4);
      setState(418);
      typeType(0);
      setState(421);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(419);
        match(TzdLangParser::ASSIGN);
        setState(420);
        expression(0);
      }
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ForInitContext ------------------------------------------------------------------

TzdLangParser::ForInitContext::ForInitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

TzdLangParser::VariableDeclarationContext* TzdLangParser::ForInitContext::variableDeclaration() {
  return getRuleContext<TzdLangParser::VariableDeclarationContext>(0);
}

TzdLangParser::ExpressionContext* TzdLangParser::ForInitContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}


size_t TzdLangParser::ForInitContext::getRuleIndex() const {
  return TzdLangParser::RuleForInit;
}


std::any TzdLangParser::ForInitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitForInit(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ForInitContext* TzdLangParser::forInit() {
  ForInitContext *_localctx = _tracker.createInstance<ForInitContext>(_ctx, getState());
  enterRule(_localctx, 40, TzdLangParser::RuleForInit);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(427);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 50, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(425);
      variableDeclaration();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(426);
      expression(0);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ForEachInitContext ------------------------------------------------------------------

TzdLangParser::ForEachInitContext::ForEachInitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::ForEachInitContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::ForEachInitContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

tree::TerminalNode* TzdLangParser::ForEachInitContext::KW_VAR() {
  return getToken(TzdLangParser::KW_VAR, 0);
}


size_t TzdLangParser::ForEachInitContext::getRuleIndex() const {
  return TzdLangParser::RuleForEachInit;
}


std::any TzdLangParser::ForEachInitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitForEachInit(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ForEachInitContext* TzdLangParser::forEachInit() {
  ForEachInitContext *_localctx = _tracker.createInstance<ForEachInitContext>(_ctx, getState());
  enterRule(_localctx, 42, TzdLangParser::RuleForEachInit);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(431);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 51, _ctx)) {
    case 1: {
      setState(429);
      typeType(0);
      break;
    }

    case 2: {
      setState(430);
      match(TzdLangParser::KW_VAR);
      break;
    }

    default:
      break;
    }
    setState(433);
    match(TzdLangParser::IDENTIFIER);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ImportStatementContext ------------------------------------------------------------------

TzdLangParser::ImportStatementContext::ImportStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::ImportStatementContext::KW_IMPORT() {
  return getToken(TzdLangParser::KW_IMPORT, 0);
}

tree::TerminalNode* TzdLangParser::ImportStatementContext::STRING() {
  return getToken(TzdLangParser::STRING, 0);
}


size_t TzdLangParser::ImportStatementContext::getRuleIndex() const {
  return TzdLangParser::RuleImportStatement;
}


std::any TzdLangParser::ImportStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitImportStatement(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ImportStatementContext* TzdLangParser::importStatement() {
  ImportStatementContext *_localctx = _tracker.createInstance<ImportStatementContext>(_ctx, getState());
  enterRule(_localctx, 44, TzdLangParser::RuleImportStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(435);
    match(TzdLangParser::KW_IMPORT);
    setState(436);
    match(TzdLangParser::STRING);
    setState(437);
    match(TzdLangParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParamListContext ------------------------------------------------------------------

TzdLangParser::ParamListContext::ParamListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::ParamContext *> TzdLangParser::ParamListContext::param() {
  return getRuleContexts<TzdLangParser::ParamContext>();
}

TzdLangParser::ParamContext* TzdLangParser::ParamListContext::param(size_t i) {
  return getRuleContext<TzdLangParser::ParamContext>(i);
}


size_t TzdLangParser::ParamListContext::getRuleIndex() const {
  return TzdLangParser::RuleParamList;
}


std::any TzdLangParser::ParamListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitParamList(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ParamListContext* TzdLangParser::paramList() {
  ParamListContext *_localctx = _tracker.createInstance<ParamListContext>(_ctx, getState());
  enterRule(_localctx, 46, TzdLangParser::RuleParamList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(439);
    param();
    setState(444);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(440);
      match(TzdLangParser::T__8);
      setState(441);
      param();
      setState(446);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParamContext ------------------------------------------------------------------

TzdLangParser::ParamContext::ParamContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

TzdLangParser::TypeTypeContext* TzdLangParser::ParamContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

tree::TerminalNode* TzdLangParser::ParamContext::ELLIPSIS() {
  return getToken(TzdLangParser::ELLIPSIS, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_INT() {
  return getToken(TzdLangParser::T_INT, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_STRING() {
  return getToken(TzdLangParser::T_STRING, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_FLOAT() {
  return getToken(TzdLangParser::T_FLOAT, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_BOOL() {
  return getToken(TzdLangParser::T_BOOL, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_VOID() {
  return getToken(TzdLangParser::T_VOID, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_PTR() {
  return getToken(TzdLangParser::T_PTR, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::T_FUNCTION() {
  return getToken(TzdLangParser::T_FUNCTION, 0);
}

tree::TerminalNode* TzdLangParser::ParamContext::KW_RET() {
  return getToken(TzdLangParser::KW_RET, 0);
}


size_t TzdLangParser::ParamContext::getRuleIndex() const {
  return TzdLangParser::RuleParam;
}


std::any TzdLangParser::ParamContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitParam(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ParamContext* TzdLangParser::param() {
  ParamContext *_localctx = _tracker.createInstance<ParamContext>(_ctx, getState());
  enterRule(_localctx, 48, TzdLangParser::RuleParam);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(461);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 54, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(447);
      typeType(0);
      setState(448);
      match(TzdLangParser::ELLIPSIS);
      setState(449);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(451);
      match(TzdLangParser::ELLIPSIS);
      setState(452);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(453);
      typeType(0);
      setState(454);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(456);
      _la = _input->LA(1);
      if (!(((((_la - 32) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 32)) & -9223372036838129663) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(459);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__4) {
        setState(457);
        match(TzdLangParser::T__4);
        setState(458);
        typeType(0);
      }
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BlockContext ------------------------------------------------------------------

TzdLangParser::BlockContext::BlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::StatementContext *> TzdLangParser::BlockContext::statement() {
  return getRuleContexts<TzdLangParser::StatementContext>();
}

TzdLangParser::StatementContext* TzdLangParser::BlockContext::statement(size_t i) {
  return getRuleContext<TzdLangParser::StatementContext>(i);
}


size_t TzdLangParser::BlockContext::getRuleIndex() const {
  return TzdLangParser::RuleBlock;
}


std::any TzdLangParser::BlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBlock(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::BlockContext* TzdLangParser::block() {
  BlockContext *_localctx = _tracker.createInstance<BlockContext>(_ctx, getState());
  enterRule(_localctx, 50, TzdLangParser::RuleBlock);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(463);
    match(TzdLangParser::T__5);
    setState(467);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 576175959595589964) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 80)) & 492641) != 0)) {
      setState(464);
      statement();
      setState(469);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(470);
    match(TzdLangParser::T__6);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t TzdLangParser::ExpressionContext::getRuleIndex() const {
  return TzdLangParser::RuleExpression;
}

void TzdLangParser::ExpressionContext::copyFrom(ExpressionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- TypeCheckExprContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::TypeCheckExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

tree::TerminalNode* TzdLangParser::TypeCheckExprContext::KW_IN() {
  return getToken(TzdLangParser::KW_IN, 0);
}

TzdLangParser::QualifiedNameContext* TzdLangParser::TypeCheckExprContext::qualifiedName() {
  return getRuleContext<TzdLangParser::QualifiedNameContext>(0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::TypeCheckExprContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::TypeCheckExprContext::TypeCheckExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::TypeCheckExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitTypeCheckExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BitAndExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::BitAndExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::BitAndExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::BitAndExprContext::BIT_AND() {
  return getToken(TzdLangParser::BIT_AND, 0);
}

TzdLangParser::BitAndExprContext::BitAndExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BitAndExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBitAndExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- RelationalExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::RelationalExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::RelationalExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::RelationalExprContext::GT() {
  return getToken(TzdLangParser::GT, 0);
}

tree::TerminalNode* TzdLangParser::RelationalExprContext::LT() {
  return getToken(TzdLangParser::LT, 0);
}

tree::TerminalNode* TzdLangParser::RelationalExprContext::GE() {
  return getToken(TzdLangParser::GE, 0);
}

tree::TerminalNode* TzdLangParser::RelationalExprContext::LE() {
  return getToken(TzdLangParser::LE, 0);
}

TzdLangParser::RelationalExprContext::RelationalExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::RelationalExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitRelationalExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AssignmentExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::AssignmentExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::AssignmentExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::ASSIGN() {
  return getToken(TzdLangParser::ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::PLUS_ASSIGN() {
  return getToken(TzdLangParser::PLUS_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::MIN_ASSIGN() {
  return getToken(TzdLangParser::MIN_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::MUL_ASSIGN() {
  return getToken(TzdLangParser::MUL_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::DIV_ASSIGN() {
  return getToken(TzdLangParser::DIV_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::MOD_ASSIGN() {
  return getToken(TzdLangParser::MOD_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::AND_ASSIGN() {
  return getToken(TzdLangParser::AND_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::OR_ASSIGN() {
  return getToken(TzdLangParser::OR_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::XOR_ASSIGN() {
  return getToken(TzdLangParser::XOR_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::SHL_ASSIGN() {
  return getToken(TzdLangParser::SHL_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::SHR_ASSIGN() {
  return getToken(TzdLangParser::SHR_ASSIGN, 0);
}

tree::TerminalNode* TzdLangParser::AssignmentExprContext::USHR_ASSIGN() {
  return getToken(TzdLangParser::USHR_ASSIGN, 0);
}

TzdLangParser::AssignmentExprContext::AssignmentExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::AssignmentExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAssignmentExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AtomExprContext ------------------------------------------------------------------

TzdLangParser::AtomContext* TzdLangParser::AtomExprContext::atom() {
  return getRuleContext<TzdLangParser::AtomContext>(0);
}

TzdLangParser::AtomExprContext::AtomExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::AtomExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAtomExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BitOrExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::BitOrExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::BitOrExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::BitOrExprContext::BIT_OR() {
  return getToken(TzdLangParser::BIT_OR, 0);
}

TzdLangParser::BitOrExprContext::BitOrExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BitOrExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBitOrExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryExprContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::UnaryExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::PLUS() {
  return getToken(TzdLangParser::PLUS, 0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::MINUS() {
  return getToken(TzdLangParser::MINUS, 0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::NOT() {
  return getToken(TzdLangParser::NOT, 0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::BIT_NOT() {
  return getToken(TzdLangParser::BIT_NOT, 0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::GXXX() {
  return getToken(TzdLangParser::GXXX, 0);
}

TzdLangParser::UnaryExprContext::UnaryExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::UnaryExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitUnaryExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LogicalAndExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::LogicalAndExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::LogicalAndExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::LogicalAndExprContext::AND() {
  return getToken(TzdLangParser::AND, 0);
}

TzdLangParser::LogicalAndExprContext::LogicalAndExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::LogicalAndExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitLogicalAndExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IndexExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::IndexExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::IndexExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

TzdLangParser::IndexExprContext::IndexExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::IndexExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitIndexExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PrefixExprContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::PrefixExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

tree::TerminalNode* TzdLangParser::PrefixExprContext::INC() {
  return getToken(TzdLangParser::INC, 0);
}

tree::TerminalNode* TzdLangParser::PrefixExprContext::DEC() {
  return getToken(TzdLangParser::DEC, 0);
}

TzdLangParser::PrefixExprContext::PrefixExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::PrefixExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitPrefixExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PostfixExprContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::PostfixExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

tree::TerminalNode* TzdLangParser::PostfixExprContext::INC() {
  return getToken(TzdLangParser::INC, 0);
}

tree::TerminalNode* TzdLangParser::PostfixExprContext::DEC() {
  return getToken(TzdLangParser::DEC, 0);
}

TzdLangParser::PostfixExprContext::PostfixExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::PostfixExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitPostfixExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PowerExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::PowerExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::PowerExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::PowerExprContext::POW() {
  return getToken(TzdLangParser::POW, 0);
}

TzdLangParser::PowerExprContext::PowerExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::PowerExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitPowerExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MultiplicativeExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::MultiplicativeExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::MultiplicativeExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::MultiplicativeExprContext::MUL() {
  return getToken(TzdLangParser::MUL, 0);
}

tree::TerminalNode* TzdLangParser::MultiplicativeExprContext::DIV() {
  return getToken(TzdLangParser::DIV, 0);
}

tree::TerminalNode* TzdLangParser::MultiplicativeExprContext::MOD() {
  return getToken(TzdLangParser::MOD, 0);
}

TzdLangParser::MultiplicativeExprContext::MultiplicativeExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::MultiplicativeExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMultiplicativeExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LogicalOrExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::LogicalOrExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::LogicalOrExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::LogicalOrExprContext::OR() {
  return getToken(TzdLangParser::OR, 0);
}

TzdLangParser::LogicalOrExprContext::LogicalOrExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::LogicalOrExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitLogicalOrExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- EqualityExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::EqualityExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::EqualityExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::EqualityExprContext::EEQ() {
  return getToken(TzdLangParser::EEQ, 0);
}

tree::TerminalNode* TzdLangParser::EqualityExprContext::NEQ() {
  return getToken(TzdLangParser::NEQ, 0);
}

TzdLangParser::EqualityExprContext::EqualityExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::EqualityExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitEqualityExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AdditiveExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::AdditiveExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::AdditiveExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::AdditiveExprContext::PLUS() {
  return getToken(TzdLangParser::PLUS, 0);
}

tree::TerminalNode* TzdLangParser::AdditiveExprContext::MINUS() {
  return getToken(TzdLangParser::MINUS, 0);
}

TzdLangParser::AdditiveExprContext::AdditiveExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::AdditiveExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitAdditiveExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- CastExprContext ------------------------------------------------------------------

TzdLangParser::TypeTypeContext* TzdLangParser::CastExprContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::ExpressionContext* TzdLangParser::CastExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::CastExprContext::CastExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::CastExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitCastExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ShiftExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::ShiftExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::ShiftExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::ShiftExprContext::SHL() {
  return getToken(TzdLangParser::SHL, 0);
}

tree::TerminalNode* TzdLangParser::ShiftExprContext::SHR() {
  return getToken(TzdLangParser::SHR, 0);
}

tree::TerminalNode* TzdLangParser::ShiftExprContext::USHR() {
  return getToken(TzdLangParser::USHR, 0);
}

TzdLangParser::ShiftExprContext::ShiftExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ShiftExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitShiftExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BitXorExprContext ------------------------------------------------------------------

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::BitXorExprContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::BitXorExprContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}

tree::TerminalNode* TzdLangParser::BitXorExprContext::BIT_XOR() {
  return getToken(TzdLangParser::BIT_XOR, 0);
}

TzdLangParser::BitXorExprContext::BitXorExprContext(ExpressionContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BitXorExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBitXorExpr(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ExpressionContext* TzdLangParser::expression() {
   return expression(0);
}

TzdLangParser::ExpressionContext* TzdLangParser::expression(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  TzdLangParser::ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, parentState);
  TzdLangParser::ExpressionContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 52;
  enterRecursionRule(_localctx, 52, TzdLangParser::RuleExpression, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(483);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 56, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<CastExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(473);
      match(TzdLangParser::T__2);
      setState(474);
      typeType(0);
      setState(475);
      match(TzdLangParser::T__3);
      setState(476);
      expression(19);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<PrefixExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(478);
      _la = _input->LA(1);
      if (!(_la == TzdLangParser::INC

      || _la == TzdLangParser::DEC)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(479);
      expression(15);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<UnaryExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(480);
      _la = _input->LA(1);
      if (!(((((_la - 58) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 58)) & 4701814785) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(481);
      expression(14);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<AtomExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(482);
      atom(0);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(536);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 59, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(534);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 58, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<PowerExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(485);

          if (!(precpred(_ctx, 16))) throw FailedPredicateException(this, "precpred(_ctx, 16)");
          setState(486);
          match(TzdLangParser::POW);
          setState(487);
          expression(16);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<MultiplicativeExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(488);

          if (!(precpred(_ctx, 12))) throw FailedPredicateException(this, "precpred(_ctx, 12)");
          setState(489);
          _la = _input->LA(1);
          if (!(((((_la - 87) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 87)) & 7) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(490);
          expression(13);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<AdditiveExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(491);

          if (!(precpred(_ctx, 11))) throw FailedPredicateException(this, "precpred(_ctx, 11)");
          setState(492);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::PLUS

          || _la == TzdLangParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(493);
          expression(12);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ShiftExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(494);

          if (!(precpred(_ctx, 10))) throw FailedPredicateException(this, "precpred(_ctx, 10)");
          setState(495);
          _la = _input->LA(1);
          if (!(((((_la - 62) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 62)) & 7) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(496);
          expression(11);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<RelationalExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(497);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(498);
          _la = _input->LA(1);
          if (!(((((_la - 75) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 75)) & 196611) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(499);
          expression(10);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<EqualityExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(500);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(501);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::EEQ

          || _la == TzdLangParser::NEQ)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(502);
          expression(9);
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<BitAndExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(503);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(504);
          match(TzdLangParser::BIT_AND);
          setState(505);
          expression(8);
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<BitXorExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(506);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(507);
          match(TzdLangParser::BIT_XOR);
          setState(508);
          expression(7);
          break;
        }

        case 9: {
          auto newContext = _tracker.createInstance<BitOrExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(509);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(510);
          match(TzdLangParser::BIT_OR);
          setState(511);
          expression(6);
          break;
        }

        case 10: {
          auto newContext = _tracker.createInstance<LogicalAndExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(512);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(513);
          match(TzdLangParser::AND);
          setState(514);
          expression(5);
          break;
        }

        case 11: {
          auto newContext = _tracker.createInstance<LogicalOrExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(515);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(516);
          match(TzdLangParser::OR);
          setState(517);
          expression(4);
          break;
        }

        case 12: {
          auto newContext = _tracker.createInstance<AssignmentExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(518);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(519);
          _la = _input->LA(1);
          if (!(((((_la - 59) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 59)) & 17179885511) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(520);
          expression(2);
          break;
        }

        case 13: {
          auto newContext = _tracker.createInstance<IndexExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(521);

          if (!(precpred(_ctx, 18))) throw FailedPredicateException(this, "precpred(_ctx, 18)");
          setState(522);
          match(TzdLangParser::T__12);
          setState(523);
          expression(0);
          setState(524);
          match(TzdLangParser::T__13);
          break;
        }

        case 14: {
          auto newContext = _tracker.createInstance<PostfixExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(526);

          if (!(precpred(_ctx, 17))) throw FailedPredicateException(this, "precpred(_ctx, 17)");
          setState(527);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::INC

          || _la == TzdLangParser::DEC)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          break;
        }

        case 15: {
          auto newContext = _tracker.createInstance<TypeCheckExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(528);

          if (!(precpred(_ctx, 13))) throw FailedPredicateException(this, "precpred(_ctx, 13)");
          setState(529);
          match(TzdLangParser::KW_IN);
          setState(532);
          _errHandler->sync(this);
          switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 57, _ctx)) {
          case 1: {
            setState(530);
            qualifiedName();
            break;
          }

          case 2: {
            setState(531);
            typeType(0);
            break;
          }

          default:
            break;
          }
          break;
        }

        default:
          break;
        } 
      }
      setState(538);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 59, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- AtomContext ------------------------------------------------------------------

TzdLangParser::AtomContext::AtomContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t TzdLangParser::AtomContext::getRuleIndex() const {
  return TzdLangParser::RuleAtom;
}

void TzdLangParser::AtomContext::copyFrom(AtomContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- BoolTrueExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::BoolTrueExprContext::KW_TRUE() {
  return getToken(TzdLangParser::KW_TRUE, 0);
}

TzdLangParser::BoolTrueExprContext::BoolTrueExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BoolTrueExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBoolTrueExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StringExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::StringExprContext::STRING() {
  return getToken(TzdLangParser::STRING, 0);
}

TzdLangParser::StringExprContext::StringExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::StringExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitStringExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FloatExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::FloatExprContext::FLOAT() {
  return getToken(TzdLangParser::FLOAT, 0);
}

TzdLangParser::FloatExprContext::FloatExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::FloatExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitFloatExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BoolFalseExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::BoolFalseExprContext::KW_FALSE() {
  return getToken(TzdLangParser::KW_FALSE, 0);
}

TzdLangParser::BoolFalseExprContext::BoolFalseExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::BoolFalseExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitBoolFalseExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IdExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::IdExprContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::IdExprContext::IdExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::IdExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitIdExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- SuperExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::SuperExprContext::KW_SUPER() {
  return getToken(TzdLangParser::KW_SUPER, 0);
}

TzdLangParser::ExprListContext* TzdLangParser::SuperExprContext::exprList() {
  return getRuleContext<TzdLangParser::ExprListContext>(0);
}

TzdLangParser::SuperExprContext::SuperExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::SuperExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitSuperExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LambdaExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::LambdaExprContext::KW_FUN() {
  return getToken(TzdLangParser::KW_FUN, 0);
}

TzdLangParser::BlockContext* TzdLangParser::LambdaExprContext::block() {
  return getRuleContext<TzdLangParser::BlockContext>(0);
}

TzdLangParser::ParamListContext* TzdLangParser::LambdaExprContext::paramList() {
  return getRuleContext<TzdLangParser::ParamListContext>(0);
}

tree::TerminalNode* TzdLangParser::LambdaExprContext::ARROW() {
  return getToken(TzdLangParser::ARROW, 0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::LambdaExprContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}

TzdLangParser::LambdaExprContext::LambdaExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::LambdaExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitLambdaExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NullExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::NullExprContext::KW_NULL() {
  return getToken(TzdLangParser::KW_NULL, 0);
}

TzdLangParser::NullExprContext::NullExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::NullExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNullExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PrintFunExprContext ------------------------------------------------------------------

TzdLangParser::PrintFunctionContext* TzdLangParser::PrintFunExprContext::printFunction() {
  return getRuleContext<TzdLangParser::PrintFunctionContext>(0);
}

TzdLangParser::PrintFunExprContext::PrintFunExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::PrintFunExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitPrintFunExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ArrayLiteralExprContext ------------------------------------------------------------------

TzdLangParser::ExprListContext* TzdLangParser::ArrayLiteralExprContext::exprList() {
  return getRuleContext<TzdLangParser::ExprListContext>(0);
}

TzdLangParser::ArrayLiteralExprContext::ArrayLiteralExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ArrayLiteralExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitArrayLiteralExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MapLiteralExprContext ------------------------------------------------------------------

TzdLangParser::MapEntryListContext* TzdLangParser::MapLiteralExprContext::mapEntryList() {
  return getRuleContext<TzdLangParser::MapEntryListContext>(0);
}

TzdLangParser::MapLiteralExprContext::MapLiteralExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::MapLiteralExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMapLiteralExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NewExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::NewExprContext::KW_NEW() {
  return getToken(TzdLangParser::KW_NEW, 0);
}

TzdLangParser::QualifiedNameContext* TzdLangParser::NewExprContext::qualifiedName() {
  return getRuleContext<TzdLangParser::QualifiedNameContext>(0);
}

TzdLangParser::ExprListContext* TzdLangParser::NewExprContext::exprList() {
  return getRuleContext<TzdLangParser::ExprListContext>(0);
}

TzdLangParser::ClassOverrideBlockContext* TzdLangParser::NewExprContext::classOverrideBlock() {
  return getRuleContext<TzdLangParser::ClassOverrideBlockContext>(0);
}

TzdLangParser::NewExprContext::NewExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::NewExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitNewExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- CallExprContext ------------------------------------------------------------------

TzdLangParser::AtomContext* TzdLangParser::CallExprContext::atom() {
  return getRuleContext<TzdLangParser::AtomContext>(0);
}

TzdLangParser::ExprListContext* TzdLangParser::CallExprContext::exprList() {
  return getRuleContext<TzdLangParser::ExprListContext>(0);
}

TzdLangParser::CallExprContext::CallExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::CallExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitCallExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IntExprContext ------------------------------------------------------------------

tree::TerminalNode* TzdLangParser::IntExprContext::INTEGER() {
  return getToken(TzdLangParser::INTEGER, 0);
}

TzdLangParser::IntExprContext::IntExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::IntExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitIntExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ParenExprContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::ParenExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

TzdLangParser::ParenExprContext::ParenExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::ParenExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitParenExpr(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MemberAccessExprContext ------------------------------------------------------------------

TzdLangParser::AtomContext* TzdLangParser::MemberAccessExprContext::atom() {
  return getRuleContext<TzdLangParser::AtomContext>(0);
}

tree::TerminalNode* TzdLangParser::MemberAccessExprContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

TzdLangParser::MemberAccessExprContext::MemberAccessExprContext(AtomContext *ctx) { copyFrom(ctx); }


std::any TzdLangParser::MemberAccessExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMemberAccessExpr(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::AtomContext* TzdLangParser::atom() {
   return atom(0);
}

TzdLangParser::AtomContext* TzdLangParser::atom(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  TzdLangParser::AtomContext *_localctx = _tracker.createInstance<AtomContext>(_ctx, parentState);
  TzdLangParser::AtomContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 54;
  enterRecursionRule(_localctx, 54, TzdLangParser::RuleAtom, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(594);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 68, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ParenExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(540);
      match(TzdLangParser::T__2);
      setState(541);
      expression(0);
      setState(542);
      match(TzdLangParser::T__3);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ArrayLiteralExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(544);
      match(TzdLangParser::T__12);
      setState(546);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(545);
        exprList();
      }
      setState(548);
      match(TzdLangParser::T__13);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<MapLiteralExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(549);
      match(TzdLangParser::T__5);
      setState(551);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__2 || ((((_la - 95) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 95)) & 11) != 0)) {
        setState(550);
        mapEntryList();
      }
      setState(553);
      match(TzdLangParser::T__6);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ArrayLiteralExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(554);
      match(TzdLangParser::T__5);
      setState(556);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(555);
        exprList();
      }
      setState(558);
      match(TzdLangParser::T__6);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<IntExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(559);
      match(TzdLangParser::INTEGER);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<FloatExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(560);
      match(TzdLangParser::FLOAT);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<StringExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(561);
      match(TzdLangParser::STRING);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<BoolTrueExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(562);
      match(TzdLangParser::KW_TRUE);
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<BoolFalseExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(563);
      match(TzdLangParser::KW_FALSE);
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<NullExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(564);
      match(TzdLangParser::KW_NULL);
      break;
    }

    case 11: {
      _localctx = _tracker.createInstance<IdExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(565);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<SuperExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(566);
      match(TzdLangParser::KW_SUPER);
      setState(567);
      match(TzdLangParser::T__2);
      setState(569);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(568);
        exprList();
      }
      setState(571);
      match(TzdLangParser::T__3);
      break;
    }

    case 13: {
      _localctx = _tracker.createInstance<PrintFunExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(572);
      printFunction();
      break;
    }

    case 14: {
      _localctx = _tracker.createInstance<NewExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(573);
      match(TzdLangParser::KW_NEW);
      setState(574);
      qualifiedName();
      setState(575);
      match(TzdLangParser::T__2);
      setState(577);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 80)) & 492641) != 0)) {
        setState(576);
        exprList();
      }
      setState(579);
      match(TzdLangParser::T__3);
      setState(581);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 65, _ctx)) {
      case 1: {
        setState(580);
        classOverrideBlock();
        break;
      }

      default:
        break;
      }
      break;
    }

    case 15: {
      _localctx = _tracker.createInstance<LambdaExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(583);
      match(TzdLangParser::KW_FUN);
      setState(584);
      match(TzdLangParser::T__2);
      setState(586);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (((((_la - 32) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 32)) & -4611686018410741759) != 0)) {
        setState(585);
        paramList();
      }
      setState(588);
      match(TzdLangParser::T__3);
      setState(591);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ARROW) {
        setState(589);
        match(TzdLangParser::ARROW);
        setState(590);
        typeType(0);
      }
      setState(593);
      block();
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(607);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 71, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(605);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 70, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<CallExprContext>(_tracker.createInstance<AtomContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleAtom);
          setState(596);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(597);
          match(TzdLangParser::T__2);
          setState(599);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if ((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 80)) & 492641) != 0)) {
            setState(598);
            exprList();
          }
          setState(601);
          match(TzdLangParser::T__3);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<MemberAccessExprContext>(_tracker.createInstance<AtomContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleAtom);
          setState(602);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(603);
          match(TzdLangParser::T__0);
          setState(604);
          match(TzdLangParser::IDENTIFIER);
          break;
        }

        default:
          break;
        } 
      }
      setState(609);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 71, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- ClassOverrideBlockContext ------------------------------------------------------------------

TzdLangParser::ClassOverrideBlockContext::ClassOverrideBlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::ClassMemberContext *> TzdLangParser::ClassOverrideBlockContext::classMember() {
  return getRuleContexts<TzdLangParser::ClassMemberContext>();
}

TzdLangParser::ClassMemberContext* TzdLangParser::ClassOverrideBlockContext::classMember(size_t i) {
  return getRuleContext<TzdLangParser::ClassMemberContext>(i);
}


size_t TzdLangParser::ClassOverrideBlockContext::getRuleIndex() const {
  return TzdLangParser::RuleClassOverrideBlock;
}


std::any TzdLangParser::ClassOverrideBlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitClassOverrideBlock(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ClassOverrideBlockContext* TzdLangParser::classOverrideBlock() {
  ClassOverrideBlockContext *_localctx = _tracker.createInstance<ClassOverrideBlockContext>(_ctx, getState());
  enterRule(_localctx, 56, TzdLangParser::RuleClassOverrideBlock);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(610);
    match(TzdLangParser::T__5);
    setState(614);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 4035936512) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(611);
      classMember();
      setState(616);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(617);
    match(TzdLangParser::T__6);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PrintFunctionContext ------------------------------------------------------------------

TzdLangParser::PrintFunctionContext::PrintFunctionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::PrintFunctionContext::KW_PRINT() {
  return getToken(TzdLangParser::KW_PRINT, 0);
}

TzdLangParser::ExprListContext* TzdLangParser::PrintFunctionContext::exprList() {
  return getRuleContext<TzdLangParser::ExprListContext>(0);
}


size_t TzdLangParser::PrintFunctionContext::getRuleIndex() const {
  return TzdLangParser::RulePrintFunction;
}


std::any TzdLangParser::PrintFunctionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitPrintFunction(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::PrintFunctionContext* TzdLangParser::printFunction() {
  PrintFunctionContext *_localctx = _tracker.createInstance<PrintFunctionContext>(_ctx, getState());
  enterRule(_localctx, 58, TzdLangParser::RulePrintFunction);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(619);
    match(TzdLangParser::KW_PRINT);
    setState(620);
    match(TzdLangParser::T__2);
    setState(622);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 504469131148402760) != 0) || ((((_la - 80) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 80)) & 492641) != 0)) {
      setState(621);
      exprList();
    }
    setState(624);
    match(TzdLangParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExprListContext ------------------------------------------------------------------

TzdLangParser::ExprListContext::ExprListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::ExpressionContext *> TzdLangParser::ExprListContext::expression() {
  return getRuleContexts<TzdLangParser::ExpressionContext>();
}

TzdLangParser::ExpressionContext* TzdLangParser::ExprListContext::expression(size_t i) {
  return getRuleContext<TzdLangParser::ExpressionContext>(i);
}


size_t TzdLangParser::ExprListContext::getRuleIndex() const {
  return TzdLangParser::RuleExprList;
}


std::any TzdLangParser::ExprListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitExprList(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::ExprListContext* TzdLangParser::exprList() {
  ExprListContext *_localctx = _tracker.createInstance<ExprListContext>(_ctx, getState());
  enterRule(_localctx, 60, TzdLangParser::RuleExprList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(626);
    expression(0);
    setState(631);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(627);
      match(TzdLangParser::T__8);
      setState(628);
      expression(0);
      setState(633);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MapEntryListContext ------------------------------------------------------------------

TzdLangParser::MapEntryListContext::MapEntryListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<TzdLangParser::MapEntryContext *> TzdLangParser::MapEntryListContext::mapEntry() {
  return getRuleContexts<TzdLangParser::MapEntryContext>();
}

TzdLangParser::MapEntryContext* TzdLangParser::MapEntryListContext::mapEntry(size_t i) {
  return getRuleContext<TzdLangParser::MapEntryContext>(i);
}


size_t TzdLangParser::MapEntryListContext::getRuleIndex() const {
  return TzdLangParser::RuleMapEntryList;
}


std::any TzdLangParser::MapEntryListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMapEntryList(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::MapEntryListContext* TzdLangParser::mapEntryList() {
  MapEntryListContext *_localctx = _tracker.createInstance<MapEntryListContext>(_ctx, getState());
  enterRule(_localctx, 62, TzdLangParser::RuleMapEntryList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(634);
    mapEntry();
    setState(639);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 75, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(635);
        match(TzdLangParser::T__8);
        setState(636);
        mapEntry(); 
      }
      setState(641);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 75, _ctx);
    }
    setState(643);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__8) {
      setState(642);
      match(TzdLangParser::T__8);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MapEntryContext ------------------------------------------------------------------

TzdLangParser::MapEntryContext::MapEntryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

TzdLangParser::MapKeyContext* TzdLangParser::MapEntryContext::mapKey() {
  return getRuleContext<TzdLangParser::MapKeyContext>(0);
}

TzdLangParser::ExpressionContext* TzdLangParser::MapEntryContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}


size_t TzdLangParser::MapEntryContext::getRuleIndex() const {
  return TzdLangParser::RuleMapEntry;
}


std::any TzdLangParser::MapEntryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMapEntry(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::MapEntryContext* TzdLangParser::mapEntry() {
  MapEntryContext *_localctx = _tracker.createInstance<MapEntryContext>(_ctx, getState());
  enterRule(_localctx, 64, TzdLangParser::RuleMapEntry);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(645);
    mapKey();
    setState(646);
    match(TzdLangParser::T__4);
    setState(647);
    expression(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MapKeyContext ------------------------------------------------------------------

TzdLangParser::MapKeyContext::MapKeyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::MapKeyContext::STRING() {
  return getToken(TzdLangParser::STRING, 0);
}

tree::TerminalNode* TzdLangParser::MapKeyContext::IDENTIFIER() {
  return getToken(TzdLangParser::IDENTIFIER, 0);
}

tree::TerminalNode* TzdLangParser::MapKeyContext::INTEGER() {
  return getToken(TzdLangParser::INTEGER, 0);
}

TzdLangParser::ExpressionContext* TzdLangParser::MapKeyContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}


size_t TzdLangParser::MapKeyContext::getRuleIndex() const {
  return TzdLangParser::RuleMapKey;
}


std::any TzdLangParser::MapKeyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitMapKey(this);
  else
    return visitor->visitChildren(this);
}

TzdLangParser::MapKeyContext* TzdLangParser::mapKey() {
  MapKeyContext *_localctx = _tracker.createInstance<MapKeyContext>(_ctx, getState());
  enterRule(_localctx, 66, TzdLangParser::RuleMapKey);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(656);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::STRING: {
        enterOuterAlt(_localctx, 1);
        setState(649);
        match(TzdLangParser::STRING);
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 2);
        setState(650);
        match(TzdLangParser::IDENTIFIER);
        break;
      }

      case TzdLangParser::INTEGER: {
        enterOuterAlt(_localctx, 3);
        setState(651);
        match(TzdLangParser::INTEGER);
        break;
      }

      case TzdLangParser::T__2: {
        enterOuterAlt(_localctx, 4);
        setState(652);
        match(TzdLangParser::T__2);
        setState(653);
        expression(0);
        setState(654);
        match(TzdLangParser::T__3);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TypeTypeContext ------------------------------------------------------------------

TzdLangParser::TypeTypeContext::TypeTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_INT() {
  return getToken(TzdLangParser::T_INT, 0);
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_FLOAT() {
  return getToken(TzdLangParser::T_FLOAT, 0);
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_STRING() {
  return getToken(TzdLangParser::T_STRING, 0);
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_BOOL() {
  return getToken(TzdLangParser::T_BOOL, 0);
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_VOID() {
  return getToken(TzdLangParser::T_VOID, 0);
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_PTR() {
  return getToken(TzdLangParser::T_PTR, 0);
}

tree::TerminalNode* TzdLangParser::TypeTypeContext::T_FUNCTION() {
  return getToken(TzdLangParser::T_FUNCTION, 0);
}

TzdLangParser::QualifiedNameContext* TzdLangParser::TypeTypeContext::qualifiedName() {
  return getRuleContext<TzdLangParser::QualifiedNameContext>(0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::TypeTypeContext::typeType() {
  return getRuleContext<TzdLangParser::TypeTypeContext>(0);
}


size_t TzdLangParser::TypeTypeContext::getRuleIndex() const {
  return TzdLangParser::RuleTypeType;
}


std::any TzdLangParser::TypeTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<TzdLangVisitor*>(visitor))
    return parserVisitor->visitTypeType(this);
  else
    return visitor->visitChildren(this);
}


TzdLangParser::TypeTypeContext* TzdLangParser::typeType() {
   return typeType(0);
}

TzdLangParser::TypeTypeContext* TzdLangParser::typeType(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  TzdLangParser::TypeTypeContext *_localctx = _tracker.createInstance<TypeTypeContext>(_ctx, parentState);
  TzdLangParser::TypeTypeContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 68;
  enterRecursionRule(_localctx, 68, TzdLangParser::RuleTypeType, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(667);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::T_INT: {
        setState(659);
        match(TzdLangParser::T_INT);
        break;
      }

      case TzdLangParser::T_FLOAT: {
        setState(660);
        match(TzdLangParser::T_FLOAT);
        break;
      }

      case TzdLangParser::T_STRING: {
        setState(661);
        match(TzdLangParser::T_STRING);
        break;
      }

      case TzdLangParser::T_BOOL: {
        setState(662);
        match(TzdLangParser::T_BOOL);
        break;
      }

      case TzdLangParser::T_VOID: {
        setState(663);
        match(TzdLangParser::T_VOID);
        break;
      }

      case TzdLangParser::T_PTR: {
        setState(664);
        match(TzdLangParser::T_PTR);
        break;
      }

      case TzdLangParser::T_FUNCTION: {
        setState(665);
        match(TzdLangParser::T_FUNCTION);
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        setState(666);
        qualifiedName();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(674);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 79, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        _localctx = _tracker.createInstance<TypeTypeContext>(parentContext, parentState);
        pushNewRecursionContext(_localctx, startState, RuleTypeType);
        setState(669);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(670);
        match(TzdLangParser::T__12);
        setState(671);
        match(TzdLangParser::T__13); 
      }
      setState(676);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 79, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

bool TzdLangParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 26: return expressionSempred(antlrcpp::downCast<ExpressionContext *>(context), predicateIndex);
    case 27: return atomSempred(antlrcpp::downCast<AtomContext *>(context), predicateIndex);
    case 34: return typeTypeSempred(antlrcpp::downCast<TypeTypeContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool TzdLangParser::expressionSempred(ExpressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 16);
    case 1: return precpred(_ctx, 12);
    case 2: return precpred(_ctx, 11);
    case 3: return precpred(_ctx, 10);
    case 4: return precpred(_ctx, 9);
    case 5: return precpred(_ctx, 8);
    case 6: return precpred(_ctx, 7);
    case 7: return precpred(_ctx, 6);
    case 8: return precpred(_ctx, 5);
    case 9: return precpred(_ctx, 4);
    case 10: return precpred(_ctx, 3);
    case 11: return precpred(_ctx, 2);
    case 12: return precpred(_ctx, 18);
    case 13: return precpred(_ctx, 17);
    case 14: return precpred(_ctx, 13);

  default:
    break;
  }
  return true;
}

bool TzdLangParser::atomSempred(AtomContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 15: return precpred(_ctx, 4);
    case 16: return precpred(_ctx, 3);

  default:
    break;
  }
  return true;
}

bool TzdLangParser::typeTypeSempred(TypeTypeContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 17: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

void TzdLangParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  tzdlangParserInitialize();
#else
  ::antlr4::internal::call_once(tzdlangParserOnceFlag, tzdlangParserInitialize);
#endif
}
