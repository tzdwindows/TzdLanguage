
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
      "nativeAttr", "nativePropKey", "variableDeclaration", "forInit", "importStatement", 
      "paramList", "param", "block", "expression", "atom", "classOverrideBlock", 
      "printFunction", "exprList", "mapEntryList", "mapEntry", "mapKey", 
      "typeType"
    },
    std::vector<std::string>{
      "", "'.'", "';'", "'('", "')'", "'{'", "'}'", "':'", "'@'", "','", 
      "'type'", "'dll'", "'prototype'", "'['", "']'", "'[]'", "'var'", "'const'", 
      "'let'", "'static'", "'abstract'", "'enum'", "'in'", "'super'", "'native'", 
      "'import'", "", "'class'", "'extends'", "'public'", "'private'", "'protected'", 
      "'fun'", "", "'if'", "'else'", "'while'", "'for'", "'break'", "'continue'", 
      "'switch'", "'case'", "'default'", "'new'", "'true'", "'false'", "'null'", 
      "'throw'", "'try'", "'catch'", "'int'", "'float'", "'string'", "'bool'", 
      "'void'", "", "", "'++'", "'--'", "'gxxx'", "'>>>='", "'>>='", "'<<='", 
      "'>>>'", "'>>'", "'<<'", "'&='", "'|='", "'^='", "'%='", "'+='", "'-='", 
      "'*='", "'/='", "'=='", "'!='", "'>='", "'<='", "'&&'", "'||'", "'**'", 
      "'~'", "'&'", "'|'", "'^'", "'->'", "'+'", "'-'", "'*'", "'/'", "'%'", 
      "'!'", "'>'", "'<'", "'='"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "KW_VAR", 
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
      "MOD", "NOT", "GT", "LT", "ASSIGN", "IDENTIFIER", "INTEGER", "FLOAT", 
      "STRING", "LINE_COMMENT", "BLOCK_COMMENT", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,101,655,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,1,0,5,0,70,8,0,10,
  	0,12,0,73,9,0,1,0,1,0,1,1,1,1,1,1,5,1,80,8,1,10,1,12,1,83,9,1,1,2,1,2,
  	1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,97,8,2,1,2,1,2,1,2,1,2,1,
  	2,1,2,1,2,1,2,3,2,107,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,118,
  	8,2,1,2,1,2,3,2,122,8,2,1,2,1,2,3,2,126,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
  	2,1,2,1,2,1,2,1,2,1,2,5,2,140,8,2,10,2,12,2,143,9,2,1,2,3,2,146,8,2,1,
  	2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
  	3,2,166,8,2,1,3,1,3,1,3,1,3,5,3,172,8,3,10,3,12,3,175,9,3,1,4,1,4,1,4,
  	5,4,180,8,4,10,4,12,4,183,9,4,1,5,1,5,1,5,1,5,1,5,3,5,190,8,5,1,5,1,5,
  	1,6,1,6,1,6,1,6,3,6,198,8,6,1,6,1,6,1,7,1,7,1,7,5,7,205,8,7,10,7,12,7,
  	208,9,7,1,8,3,8,211,8,8,1,8,1,8,1,8,1,8,3,8,217,8,8,1,8,1,8,1,8,1,8,1,
  	8,3,8,224,8,8,1,8,1,8,1,8,1,8,3,8,230,8,8,1,8,1,8,1,8,1,8,3,8,236,8,8,
  	1,9,5,9,239,8,9,10,9,12,9,242,9,9,1,10,3,10,245,8,10,1,10,3,10,248,8,
  	10,1,10,1,10,3,10,252,8,10,1,11,1,11,1,11,1,11,1,11,3,11,259,8,11,1,11,
  	1,11,1,11,1,11,1,11,1,11,1,11,3,11,268,8,11,1,11,1,11,1,11,1,11,1,11,
  	1,11,3,11,276,8,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,
  	287,8,11,1,11,1,11,1,11,3,11,292,8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,
  	11,300,8,11,1,11,1,11,1,11,3,11,305,8,11,1,11,1,11,1,11,1,11,1,11,3,11,
  	312,8,11,1,11,1,11,1,11,3,11,317,8,11,1,11,1,11,1,11,1,11,3,11,323,8,
  	11,1,11,1,11,3,11,327,8,11,1,12,1,12,1,12,1,12,3,12,333,8,12,1,12,3,12,
  	336,8,12,1,13,1,13,1,14,3,14,341,8,14,1,14,1,14,1,14,1,14,3,14,347,8,
  	14,1,14,1,14,1,14,3,14,352,8,14,1,14,1,14,1,15,3,15,357,8,15,1,15,1,15,
  	1,15,1,15,1,15,3,15,364,8,15,1,15,1,15,1,15,3,15,369,8,15,1,15,1,15,3,
  	15,373,8,15,1,15,1,15,1,15,1,16,1,16,1,16,5,16,381,8,16,10,16,12,16,384,
  	9,16,1,17,1,17,3,17,388,8,17,1,17,1,17,1,17,1,18,1,18,1,19,1,19,1,19,
  	1,19,3,19,399,8,19,1,19,1,19,1,19,1,19,3,19,405,8,19,1,19,1,19,1,19,1,
  	19,1,19,3,19,412,8,19,3,19,414,8,19,1,20,1,20,3,20,418,8,20,1,21,1,21,
  	1,21,1,21,1,22,1,22,1,22,5,22,427,8,22,10,22,12,22,430,9,22,1,23,1,23,
  	1,23,1,23,1,23,1,23,3,23,438,8,23,3,23,440,8,23,1,24,1,24,5,24,444,8,
  	24,10,24,12,24,447,9,24,1,24,1,24,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,1,25,1,25,3,25,462,8,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,3,25,511,
  	8,25,5,25,513,8,25,10,25,12,25,516,9,25,1,26,1,26,1,26,1,26,1,26,1,26,
  	1,26,3,26,525,8,26,1,26,1,26,1,26,3,26,530,8,26,1,26,1,26,1,26,3,26,535,
  	8,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,548,
  	8,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,556,8,26,1,26,1,26,3,26,560,8,
  	26,1,26,1,26,1,26,3,26,565,8,26,1,26,1,26,1,26,3,26,570,8,26,1,26,3,26,
  	573,8,26,1,26,1,26,1,26,3,26,578,8,26,1,26,1,26,1,26,1,26,5,26,584,8,
  	26,10,26,12,26,587,9,26,1,27,1,27,5,27,591,8,27,10,27,12,27,594,9,27,
  	1,27,1,27,1,28,1,28,1,28,3,28,601,8,28,1,28,1,28,1,29,1,29,1,29,5,29,
  	608,8,29,10,29,12,29,611,9,29,1,30,1,30,1,30,5,30,616,8,30,10,30,12,30,
  	619,9,30,1,30,3,30,622,8,30,1,31,1,31,1,31,1,31,1,32,1,32,1,32,1,32,1,
  	32,1,32,1,32,3,32,635,8,32,1,33,1,33,1,33,1,33,1,33,1,33,1,33,1,33,1,
  	33,3,33,646,8,33,1,33,1,33,5,33,650,8,33,10,33,12,33,653,9,33,1,33,0,
  	3,50,52,66,34,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,
  	40,42,44,46,48,50,52,54,56,58,60,62,64,66,0,12,1,0,29,31,2,0,96,96,98,
  	98,3,0,10,12,32,33,50,56,3,0,33,33,50,56,95,95,1,0,57,58,4,0,59,59,81,
  	81,86,87,91,91,1,0,88,90,1,0,86,87,1,0,63,65,2,0,76,77,92,93,1,0,74,75,
  	3,0,60,62,66,73,94,94,758,0,71,1,0,0,0,2,76,1,0,0,0,4,165,1,0,0,0,6,167,
  	1,0,0,0,8,176,1,0,0,0,10,184,1,0,0,0,12,193,1,0,0,0,14,201,1,0,0,0,16,
  	235,1,0,0,0,18,240,1,0,0,0,20,251,1,0,0,0,22,326,1,0,0,0,24,328,1,0,0,
  	0,26,337,1,0,0,0,28,340,1,0,0,0,30,356,1,0,0,0,32,377,1,0,0,0,34,387,
  	1,0,0,0,36,392,1,0,0,0,38,413,1,0,0,0,40,417,1,0,0,0,42,419,1,0,0,0,44,
  	423,1,0,0,0,46,439,1,0,0,0,48,441,1,0,0,0,50,461,1,0,0,0,52,572,1,0,0,
  	0,54,588,1,0,0,0,56,597,1,0,0,0,58,604,1,0,0,0,60,612,1,0,0,0,62,623,
  	1,0,0,0,64,634,1,0,0,0,66,645,1,0,0,0,68,70,3,4,2,0,69,68,1,0,0,0,70,
  	73,1,0,0,0,71,69,1,0,0,0,71,72,1,0,0,0,72,74,1,0,0,0,73,71,1,0,0,0,74,
  	75,5,0,0,1,75,1,1,0,0,0,76,81,5,95,0,0,77,78,5,1,0,0,78,80,5,95,0,0,79,
  	77,1,0,0,0,80,83,1,0,0,0,81,79,1,0,0,0,81,82,1,0,0,0,82,3,1,0,0,0,83,
  	81,1,0,0,0,84,166,3,48,24,0,85,166,3,16,8,0,86,166,3,10,5,0,87,166,3,
  	12,6,0,88,166,3,28,14,0,89,166,3,30,15,0,90,91,3,38,19,0,91,92,5,2,0,
  	0,92,166,1,0,0,0,93,166,3,42,21,0,94,96,5,33,0,0,95,97,3,50,25,0,96,95,
  	1,0,0,0,96,97,1,0,0,0,97,98,1,0,0,0,98,166,5,2,0,0,99,100,5,34,0,0,100,
  	101,5,3,0,0,101,102,3,50,25,0,102,103,5,4,0,0,103,106,3,4,2,0,104,105,
  	5,35,0,0,105,107,3,4,2,0,106,104,1,0,0,0,106,107,1,0,0,0,107,166,1,0,
  	0,0,108,109,5,36,0,0,109,110,5,3,0,0,110,111,3,50,25,0,111,112,5,4,0,
  	0,112,113,3,4,2,0,113,166,1,0,0,0,114,115,5,37,0,0,115,117,5,3,0,0,116,
  	118,3,40,20,0,117,116,1,0,0,0,117,118,1,0,0,0,118,119,1,0,0,0,119,121,
  	5,2,0,0,120,122,3,50,25,0,121,120,1,0,0,0,121,122,1,0,0,0,122,123,1,0,
  	0,0,123,125,5,2,0,0,124,126,3,50,25,0,125,124,1,0,0,0,125,126,1,0,0,0,
  	126,127,1,0,0,0,127,128,5,4,0,0,128,166,3,4,2,0,129,130,5,38,0,0,130,
  	166,5,2,0,0,131,132,5,39,0,0,132,166,5,2,0,0,133,134,5,40,0,0,134,135,
  	5,3,0,0,135,136,3,50,25,0,136,137,5,4,0,0,137,141,5,5,0,0,138,140,3,6,
  	3,0,139,138,1,0,0,0,140,143,1,0,0,0,141,139,1,0,0,0,141,142,1,0,0,0,142,
  	145,1,0,0,0,143,141,1,0,0,0,144,146,3,8,4,0,145,144,1,0,0,0,145,146,1,
  	0,0,0,146,147,1,0,0,0,147,148,5,6,0,0,148,166,1,0,0,0,149,150,5,48,0,
  	0,150,151,3,48,24,0,151,152,5,49,0,0,152,153,5,3,0,0,153,154,5,95,0,0,
  	154,155,5,4,0,0,155,156,3,48,24,0,156,166,1,0,0,0,157,158,5,47,0,0,158,
  	159,3,50,25,0,159,160,5,2,0,0,160,166,1,0,0,0,161,162,3,50,25,0,162,163,
  	5,2,0,0,163,166,1,0,0,0,164,166,5,2,0,0,165,84,1,0,0,0,165,85,1,0,0,0,
  	165,86,1,0,0,0,165,87,1,0,0,0,165,88,1,0,0,0,165,89,1,0,0,0,165,90,1,
  	0,0,0,165,93,1,0,0,0,165,94,1,0,0,0,165,99,1,0,0,0,165,108,1,0,0,0,165,
  	114,1,0,0,0,165,129,1,0,0,0,165,131,1,0,0,0,165,133,1,0,0,0,165,149,1,
  	0,0,0,165,157,1,0,0,0,165,161,1,0,0,0,165,164,1,0,0,0,166,5,1,0,0,0,167,
  	168,5,41,0,0,168,169,3,50,25,0,169,173,5,7,0,0,170,172,3,4,2,0,171,170,
  	1,0,0,0,172,175,1,0,0,0,173,171,1,0,0,0,173,174,1,0,0,0,174,7,1,0,0,0,
  	175,173,1,0,0,0,176,177,5,42,0,0,177,181,5,7,0,0,178,180,3,4,2,0,179,
  	178,1,0,0,0,180,183,1,0,0,0,181,179,1,0,0,0,181,182,1,0,0,0,182,9,1,0,
  	0,0,183,181,1,0,0,0,184,185,5,27,0,0,185,186,5,8,0,0,186,187,5,95,0,0,
  	187,189,5,3,0,0,188,190,3,44,22,0,189,188,1,0,0,0,189,190,1,0,0,0,190,
  	191,1,0,0,0,191,192,5,4,0,0,192,11,1,0,0,0,193,194,5,21,0,0,194,195,5,
  	95,0,0,195,197,5,5,0,0,196,198,3,14,7,0,197,196,1,0,0,0,197,198,1,0,0,
  	0,198,199,1,0,0,0,199,200,5,6,0,0,200,13,1,0,0,0,201,206,5,95,0,0,202,
  	203,5,9,0,0,203,205,5,95,0,0,204,202,1,0,0,0,205,208,1,0,0,0,206,204,
  	1,0,0,0,206,207,1,0,0,0,207,15,1,0,0,0,208,206,1,0,0,0,209,211,3,24,12,
  	0,210,209,1,0,0,0,210,211,1,0,0,0,211,212,1,0,0,0,212,213,5,27,0,0,213,
  	216,3,2,1,0,214,215,5,7,0,0,215,217,3,2,1,0,216,214,1,0,0,0,216,217,1,
  	0,0,0,217,218,1,0,0,0,218,219,5,5,0,0,219,220,3,18,9,0,220,221,5,6,0,
  	0,221,236,1,0,0,0,222,224,3,24,12,0,223,222,1,0,0,0,223,224,1,0,0,0,224,
  	225,1,0,0,0,225,226,5,27,0,0,226,229,3,2,1,0,227,228,5,28,0,0,228,230,
  	3,2,1,0,229,227,1,0,0,0,229,230,1,0,0,0,230,231,1,0,0,0,231,232,5,5,0,
  	0,232,233,3,18,9,0,233,234,5,6,0,0,234,236,1,0,0,0,235,210,1,0,0,0,235,
  	223,1,0,0,0,236,17,1,0,0,0,237,239,3,20,10,0,238,237,1,0,0,0,239,242,
  	1,0,0,0,240,238,1,0,0,0,240,241,1,0,0,0,241,19,1,0,0,0,242,240,1,0,0,
  	0,243,245,3,24,12,0,244,243,1,0,0,0,244,245,1,0,0,0,245,247,1,0,0,0,246,
  	248,3,26,13,0,247,246,1,0,0,0,247,248,1,0,0,0,248,249,1,0,0,0,249,252,
  	3,22,11,0,250,252,3,30,15,0,251,244,1,0,0,0,251,250,1,0,0,0,252,21,1,
  	0,0,0,253,254,5,16,0,0,254,255,3,66,33,0,255,258,5,95,0,0,256,257,5,94,
  	0,0,257,259,3,50,25,0,258,256,1,0,0,0,258,259,1,0,0,0,259,260,1,0,0,0,
  	260,261,5,2,0,0,261,327,1,0,0,0,262,263,5,18,0,0,263,264,3,66,33,0,264,
  	267,5,95,0,0,265,266,5,94,0,0,266,268,3,50,25,0,267,265,1,0,0,0,267,268,
  	1,0,0,0,268,269,1,0,0,0,269,270,5,2,0,0,270,327,1,0,0,0,271,272,5,17,
  	0,0,272,275,5,95,0,0,273,274,5,7,0,0,274,276,3,66,33,0,275,273,1,0,0,
  	0,275,276,1,0,0,0,276,277,1,0,0,0,277,278,5,94,0,0,278,279,3,50,25,0,
  	279,280,5,2,0,0,280,327,1,0,0,0,281,282,5,19,0,0,282,283,5,32,0,0,283,
  	284,5,95,0,0,284,286,5,3,0,0,285,287,3,44,22,0,286,285,1,0,0,0,286,287,
  	1,0,0,0,287,288,1,0,0,0,288,291,5,4,0,0,289,290,5,85,0,0,290,292,3,66,
  	33,0,291,289,1,0,0,0,291,292,1,0,0,0,292,293,1,0,0,0,293,327,3,48,24,
  	0,294,295,5,20,0,0,295,296,5,32,0,0,296,297,5,95,0,0,297,299,5,3,0,0,
  	298,300,3,44,22,0,299,298,1,0,0,0,299,300,1,0,0,0,300,301,1,0,0,0,301,
  	304,5,4,0,0,302,303,5,85,0,0,303,305,3,66,33,0,304,302,1,0,0,0,304,305,
  	1,0,0,0,305,306,1,0,0,0,306,327,5,2,0,0,307,308,5,32,0,0,308,309,5,95,
  	0,0,309,311,5,3,0,0,310,312,3,44,22,0,311,310,1,0,0,0,311,312,1,0,0,0,
  	312,313,1,0,0,0,313,316,5,4,0,0,314,315,5,85,0,0,315,317,3,66,33,0,316,
  	314,1,0,0,0,316,317,1,0,0,0,317,318,1,0,0,0,318,327,3,48,24,0,319,320,
  	5,95,0,0,320,322,5,3,0,0,321,323,3,44,22,0,322,321,1,0,0,0,322,323,1,
  	0,0,0,323,324,1,0,0,0,324,325,5,4,0,0,325,327,3,48,24,0,326,253,1,0,0,
  	0,326,262,1,0,0,0,326,271,1,0,0,0,326,281,1,0,0,0,326,294,1,0,0,0,326,
  	307,1,0,0,0,326,319,1,0,0,0,327,23,1,0,0,0,328,329,5,8,0,0,329,335,5,
  	95,0,0,330,332,5,3,0,0,331,333,3,58,29,0,332,331,1,0,0,0,332,333,1,0,
  	0,0,333,334,1,0,0,0,334,336,5,4,0,0,335,330,1,0,0,0,335,336,1,0,0,0,336,
  	25,1,0,0,0,337,338,7,0,0,0,338,27,1,0,0,0,339,341,3,24,12,0,340,339,1,
  	0,0,0,340,341,1,0,0,0,341,342,1,0,0,0,342,343,5,32,0,0,343,344,5,95,0,
  	0,344,346,5,3,0,0,345,347,3,44,22,0,346,345,1,0,0,0,346,347,1,0,0,0,347,
  	348,1,0,0,0,348,351,5,4,0,0,349,350,5,85,0,0,350,352,3,66,33,0,351,349,
  	1,0,0,0,351,352,1,0,0,0,352,353,1,0,0,0,353,354,3,48,24,0,354,29,1,0,
  	0,0,355,357,3,24,12,0,356,355,1,0,0,0,356,357,1,0,0,0,357,358,1,0,0,0,
  	358,359,5,24,0,0,359,360,5,32,0,0,360,361,5,95,0,0,361,363,5,3,0,0,362,
  	364,3,44,22,0,363,362,1,0,0,0,363,364,1,0,0,0,364,365,1,0,0,0,365,368,
  	5,4,0,0,366,367,5,85,0,0,367,369,3,66,33,0,368,366,1,0,0,0,368,369,1,
  	0,0,0,369,370,1,0,0,0,370,372,5,3,0,0,371,373,3,32,16,0,372,371,1,0,0,
  	0,372,373,1,0,0,0,373,374,1,0,0,0,374,375,5,4,0,0,375,376,5,2,0,0,376,
  	31,1,0,0,0,377,382,3,34,17,0,378,379,5,9,0,0,379,381,3,34,17,0,380,378,
  	1,0,0,0,381,384,1,0,0,0,382,380,1,0,0,0,382,383,1,0,0,0,383,33,1,0,0,
  	0,384,382,1,0,0,0,385,388,5,95,0,0,386,388,3,36,18,0,387,385,1,0,0,0,
  	387,386,1,0,0,0,388,389,1,0,0,0,389,390,5,94,0,0,390,391,7,1,0,0,391,
  	35,1,0,0,0,392,393,7,2,0,0,393,37,1,0,0,0,394,395,3,66,33,0,395,398,5,
  	95,0,0,396,397,5,94,0,0,397,399,3,50,25,0,398,396,1,0,0,0,398,399,1,0,
  	0,0,399,414,1,0,0,0,400,401,5,16,0,0,401,404,5,95,0,0,402,403,5,94,0,
  	0,403,405,3,50,25,0,404,402,1,0,0,0,404,405,1,0,0,0,405,414,1,0,0,0,406,
  	407,5,95,0,0,407,408,5,7,0,0,408,411,3,66,33,0,409,410,5,94,0,0,410,412,
  	3,50,25,0,411,409,1,0,0,0,411,412,1,0,0,0,412,414,1,0,0,0,413,394,1,0,
  	0,0,413,400,1,0,0,0,413,406,1,0,0,0,414,39,1,0,0,0,415,418,3,38,19,0,
  	416,418,3,50,25,0,417,415,1,0,0,0,417,416,1,0,0,0,418,41,1,0,0,0,419,
  	420,5,25,0,0,420,421,5,98,0,0,421,422,5,2,0,0,422,43,1,0,0,0,423,428,
  	3,46,23,0,424,425,5,9,0,0,425,427,3,46,23,0,426,424,1,0,0,0,427,430,1,
  	0,0,0,428,426,1,0,0,0,428,429,1,0,0,0,429,45,1,0,0,0,430,428,1,0,0,0,
  	431,432,3,66,33,0,432,433,5,95,0,0,433,440,1,0,0,0,434,437,7,3,0,0,435,
  	436,5,7,0,0,436,438,3,66,33,0,437,435,1,0,0,0,437,438,1,0,0,0,438,440,
  	1,0,0,0,439,431,1,0,0,0,439,434,1,0,0,0,440,47,1,0,0,0,441,445,5,5,0,
  	0,442,444,3,4,2,0,443,442,1,0,0,0,444,447,1,0,0,0,445,443,1,0,0,0,445,
  	446,1,0,0,0,446,448,1,0,0,0,447,445,1,0,0,0,448,449,5,6,0,0,449,49,1,
  	0,0,0,450,451,6,25,-1,0,451,452,5,3,0,0,452,453,3,66,33,0,453,454,5,4,
  	0,0,454,455,3,50,25,19,455,462,1,0,0,0,456,457,7,4,0,0,457,462,3,50,25,
  	15,458,459,7,5,0,0,459,462,3,50,25,14,460,462,3,52,26,0,461,450,1,0,0,
  	0,461,456,1,0,0,0,461,458,1,0,0,0,461,460,1,0,0,0,462,514,1,0,0,0,463,
  	464,10,16,0,0,464,465,5,80,0,0,465,513,3,50,25,16,466,467,10,12,0,0,467,
  	468,7,6,0,0,468,513,3,50,25,13,469,470,10,11,0,0,470,471,7,7,0,0,471,
  	513,3,50,25,12,472,473,10,10,0,0,473,474,7,8,0,0,474,513,3,50,25,11,475,
  	476,10,9,0,0,476,477,7,9,0,0,477,513,3,50,25,10,478,479,10,8,0,0,479,
  	480,7,10,0,0,480,513,3,50,25,9,481,482,10,7,0,0,482,483,5,82,0,0,483,
  	513,3,50,25,8,484,485,10,6,0,0,485,486,5,84,0,0,486,513,3,50,25,7,487,
  	488,10,5,0,0,488,489,5,83,0,0,489,513,3,50,25,6,490,491,10,4,0,0,491,
  	492,5,78,0,0,492,513,3,50,25,5,493,494,10,3,0,0,494,495,5,79,0,0,495,
  	513,3,50,25,4,496,497,10,2,0,0,497,498,7,11,0,0,498,513,3,50,25,2,499,
  	500,10,18,0,0,500,501,5,13,0,0,501,502,3,50,25,0,502,503,5,14,0,0,503,
  	513,1,0,0,0,504,505,10,17,0,0,505,513,7,4,0,0,506,507,10,13,0,0,507,510,
  	5,22,0,0,508,511,3,2,1,0,509,511,3,66,33,0,510,508,1,0,0,0,510,509,1,
  	0,0,0,511,513,1,0,0,0,512,463,1,0,0,0,512,466,1,0,0,0,512,469,1,0,0,0,
  	512,472,1,0,0,0,512,475,1,0,0,0,512,478,1,0,0,0,512,481,1,0,0,0,512,484,
  	1,0,0,0,512,487,1,0,0,0,512,490,1,0,0,0,512,493,1,0,0,0,512,496,1,0,0,
  	0,512,499,1,0,0,0,512,504,1,0,0,0,512,506,1,0,0,0,513,516,1,0,0,0,514,
  	512,1,0,0,0,514,515,1,0,0,0,515,51,1,0,0,0,516,514,1,0,0,0,517,518,6,
  	26,-1,0,518,519,5,3,0,0,519,520,3,50,25,0,520,521,5,4,0,0,521,573,1,0,
  	0,0,522,524,5,13,0,0,523,525,3,58,29,0,524,523,1,0,0,0,524,525,1,0,0,
  	0,525,526,1,0,0,0,526,573,5,14,0,0,527,529,5,5,0,0,528,530,3,60,30,0,
  	529,528,1,0,0,0,529,530,1,0,0,0,530,531,1,0,0,0,531,573,5,6,0,0,532,534,
  	5,5,0,0,533,535,3,58,29,0,534,533,1,0,0,0,534,535,1,0,0,0,535,536,1,0,
  	0,0,536,573,5,6,0,0,537,573,5,96,0,0,538,573,5,97,0,0,539,573,5,98,0,
  	0,540,573,5,44,0,0,541,573,5,45,0,0,542,573,5,46,0,0,543,573,5,95,0,0,
  	544,545,5,23,0,0,545,547,5,3,0,0,546,548,3,58,29,0,547,546,1,0,0,0,547,
  	548,1,0,0,0,548,549,1,0,0,0,549,573,5,4,0,0,550,573,3,56,28,0,551,552,
  	5,43,0,0,552,553,3,2,1,0,553,555,5,3,0,0,554,556,3,58,29,0,555,554,1,
  	0,0,0,555,556,1,0,0,0,556,557,1,0,0,0,557,559,5,4,0,0,558,560,3,54,27,
  	0,559,558,1,0,0,0,559,560,1,0,0,0,560,573,1,0,0,0,561,562,5,32,0,0,562,
  	564,5,3,0,0,563,565,3,44,22,0,564,563,1,0,0,0,564,565,1,0,0,0,565,566,
  	1,0,0,0,566,569,5,4,0,0,567,568,5,85,0,0,568,570,3,66,33,0,569,567,1,
  	0,0,0,569,570,1,0,0,0,570,571,1,0,0,0,571,573,3,48,24,0,572,517,1,0,0,
  	0,572,522,1,0,0,0,572,527,1,0,0,0,572,532,1,0,0,0,572,537,1,0,0,0,572,
  	538,1,0,0,0,572,539,1,0,0,0,572,540,1,0,0,0,572,541,1,0,0,0,572,542,1,
  	0,0,0,572,543,1,0,0,0,572,544,1,0,0,0,572,550,1,0,0,0,572,551,1,0,0,0,
  	572,561,1,0,0,0,573,585,1,0,0,0,574,575,10,4,0,0,575,577,5,3,0,0,576,
  	578,3,58,29,0,577,576,1,0,0,0,577,578,1,0,0,0,578,579,1,0,0,0,579,584,
  	5,4,0,0,580,581,10,3,0,0,581,582,5,1,0,0,582,584,5,95,0,0,583,574,1,0,
  	0,0,583,580,1,0,0,0,584,587,1,0,0,0,585,583,1,0,0,0,585,586,1,0,0,0,586,
  	53,1,0,0,0,587,585,1,0,0,0,588,592,5,5,0,0,589,591,3,20,10,0,590,589,
  	1,0,0,0,591,594,1,0,0,0,592,590,1,0,0,0,592,593,1,0,0,0,593,595,1,0,0,
  	0,594,592,1,0,0,0,595,596,5,6,0,0,596,55,1,0,0,0,597,598,5,26,0,0,598,
  	600,5,3,0,0,599,601,3,58,29,0,600,599,1,0,0,0,600,601,1,0,0,0,601,602,
  	1,0,0,0,602,603,5,4,0,0,603,57,1,0,0,0,604,609,3,50,25,0,605,606,5,9,
  	0,0,606,608,3,50,25,0,607,605,1,0,0,0,608,611,1,0,0,0,609,607,1,0,0,0,
  	609,610,1,0,0,0,610,59,1,0,0,0,611,609,1,0,0,0,612,617,3,62,31,0,613,
  	614,5,9,0,0,614,616,3,62,31,0,615,613,1,0,0,0,616,619,1,0,0,0,617,615,
  	1,0,0,0,617,618,1,0,0,0,618,621,1,0,0,0,619,617,1,0,0,0,620,622,5,9,0,
  	0,621,620,1,0,0,0,621,622,1,0,0,0,622,61,1,0,0,0,623,624,3,64,32,0,624,
  	625,5,7,0,0,625,626,3,50,25,0,626,63,1,0,0,0,627,635,5,98,0,0,628,635,
  	5,95,0,0,629,635,5,96,0,0,630,631,5,3,0,0,631,632,3,50,25,0,632,633,5,
  	4,0,0,633,635,1,0,0,0,634,627,1,0,0,0,634,628,1,0,0,0,634,629,1,0,0,0,
  	634,630,1,0,0,0,635,65,1,0,0,0,636,637,6,33,-1,0,637,646,5,50,0,0,638,
  	646,5,51,0,0,639,646,5,52,0,0,640,646,5,53,0,0,641,646,5,54,0,0,642,646,
  	5,55,0,0,643,646,5,56,0,0,644,646,3,2,1,0,645,636,1,0,0,0,645,638,1,0,
  	0,0,645,639,1,0,0,0,645,640,1,0,0,0,645,641,1,0,0,0,645,642,1,0,0,0,645,
  	643,1,0,0,0,645,644,1,0,0,0,646,651,1,0,0,0,647,648,10,1,0,0,648,650,
  	5,15,0,0,649,647,1,0,0,0,650,653,1,0,0,0,651,649,1,0,0,0,651,652,1,0,
  	0,0,652,67,1,0,0,0,653,651,1,0,0,0,79,71,81,96,106,117,121,125,141,145,
  	165,173,181,189,197,206,210,216,223,229,235,240,244,247,251,258,267,275,
  	286,291,299,304,311,316,322,326,332,335,340,346,351,356,363,368,372,382,
  	387,398,404,411,413,417,428,437,439,445,461,510,512,514,524,529,534,547,
  	555,559,564,569,572,577,583,585,592,600,609,617,621,634,645,651
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
    setState(71);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1152351919191171372) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & 246881) != 0)) {
      setState(68);
      statement();
      setState(73);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(74);
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
    setState(76);
    match(TzdLangParser::IDENTIFIER);
    setState(81);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(77);
        match(TzdLangParser::T__0);
        setState(78);
        match(TzdLangParser::IDENTIFIER); 
      }
      setState(83);
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
    setState(165);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<TzdLangParser::BlockStmtContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(84);
      block();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<TzdLangParser::ClassDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(85);
      classDeclaration();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<TzdLangParser::AnnotationDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(86);
      annotationDeclaration();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<TzdLangParser::EnumDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(87);
      enumDeclaration();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<TzdLangParser::FunDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(88);
      functionDeclaration();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<TzdLangParser::NativeFunDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(89);
      nativeFunctionDeclaration();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<TzdLangParser::VarDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(90);
      variableDeclaration();
      setState(91);
      match(TzdLangParser::T__1);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<TzdLangParser::ImportStmtContext>(_localctx);
      enterOuterAlt(_localctx, 8);
      setState(93);
      importStatement();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<TzdLangParser::ReturnStmtContext>(_localctx);
      enterOuterAlt(_localctx, 9);
      setState(94);
      match(TzdLangParser::KW_RET);
      setState(96);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(95);
        expression(0);
      }
      setState(98);
      match(TzdLangParser::T__1);
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<TzdLangParser::IfStmtContext>(_localctx);
      enterOuterAlt(_localctx, 10);
      setState(99);
      match(TzdLangParser::KW_IF);
      setState(100);
      match(TzdLangParser::T__2);
      setState(101);
      expression(0);
      setState(102);
      match(TzdLangParser::T__3);
      setState(103);
      statement();
      setState(106);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
      case 1: {
        setState(104);
        match(TzdLangParser::KW_ELSE);
        setState(105);
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
      setState(108);
      match(TzdLangParser::KW_WHILE);
      setState(109);
      match(TzdLangParser::T__2);
      setState(110);
      expression(0);
      setState(111);
      match(TzdLangParser::T__3);
      setState(112);
      statement();
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<TzdLangParser::ForStmtContext>(_localctx);
      enterOuterAlt(_localctx, 12);
      setState(114);
      match(TzdLangParser::KW_FOR);
      setState(115);
      match(TzdLangParser::T__2);
      setState(117);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1151927550465876008) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(116);
        forInit();
      }
      setState(119);
      match(TzdLangParser::T__1);
      setState(121);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(120);
        antlrcpp::downCast<ForStmtContext *>(_localctx)->cond = expression(0);
      }
      setState(123);
      match(TzdLangParser::T__1);
      setState(125);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(124);
        antlrcpp::downCast<ForStmtContext *>(_localctx)->step = expression(0);
      }
      setState(127);
      match(TzdLangParser::T__3);
      setState(128);
      statement();
      break;
    }

    case 13: {
      _localctx = _tracker.createInstance<TzdLangParser::BreakStmtContext>(_localctx);
      enterOuterAlt(_localctx, 13);
      setState(129);
      match(TzdLangParser::KW_BREAK);
      setState(130);
      match(TzdLangParser::T__1);
      break;
    }

    case 14: {
      _localctx = _tracker.createInstance<TzdLangParser::ContinueStmtContext>(_localctx);
      enterOuterAlt(_localctx, 14);
      setState(131);
      match(TzdLangParser::KW_CONTINUE);
      setState(132);
      match(TzdLangParser::T__1);
      break;
    }

    case 15: {
      _localctx = _tracker.createInstance<TzdLangParser::SwitchStmtContext>(_localctx);
      enterOuterAlt(_localctx, 15);
      setState(133);
      match(TzdLangParser::KW_SWITCH);
      setState(134);
      match(TzdLangParser::T__2);
      setState(135);
      expression(0);
      setState(136);
      match(TzdLangParser::T__3);
      setState(137);
      match(TzdLangParser::T__4);
      setState(141);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == TzdLangParser::KW_CASE) {
        setState(138);
        switchCase();
        setState(143);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(145);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::KW_DEFAULT) {
        setState(144);
        switchDefault();
      }
      setState(147);
      match(TzdLangParser::T__5);
      break;
    }

    case 16: {
      _localctx = _tracker.createInstance<TzdLangParser::TryCatchStmtContext>(_localctx);
      enterOuterAlt(_localctx, 16);
      setState(149);
      match(TzdLangParser::KW_TRY);
      setState(150);
      block();
      setState(151);
      match(TzdLangParser::KW_CATCH);
      setState(152);
      match(TzdLangParser::T__2);
      setState(153);
      match(TzdLangParser::IDENTIFIER);
      setState(154);
      match(TzdLangParser::T__3);
      setState(155);
      block();
      break;
    }

    case 17: {
      _localctx = _tracker.createInstance<TzdLangParser::ThrowStmtContext>(_localctx);
      enterOuterAlt(_localctx, 17);
      setState(157);
      match(TzdLangParser::KW_THROW);
      setState(158);
      expression(0);
      setState(159);
      match(TzdLangParser::T__1);
      break;
    }

    case 18: {
      _localctx = _tracker.createInstance<TzdLangParser::ExprStmtContext>(_localctx);
      enterOuterAlt(_localctx, 18);
      setState(161);
      expression(0);
      setState(162);
      match(TzdLangParser::T__1);
      break;
    }

    case 19: {
      _localctx = _tracker.createInstance<TzdLangParser::EmptyStmtContext>(_localctx);
      enterOuterAlt(_localctx, 19);
      setState(164);
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
    setState(167);
    match(TzdLangParser::KW_CASE);
    setState(168);
    expression(0);
    setState(169);
    match(TzdLangParser::T__6);
    setState(173);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1152351919191171372) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & 246881) != 0)) {
      setState(170);
      statement();
      setState(175);
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
    setState(176);
    match(TzdLangParser::KW_DEFAULT);
    setState(177);
    match(TzdLangParser::T__6);
    setState(181);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1152351919191171372) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & 246881) != 0)) {
      setState(178);
      statement();
      setState(183);
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
    setState(184);
    match(TzdLangParser::KW_CLASS);
    setState(185);
    match(TzdLangParser::T__7);
    setState(186);
    match(TzdLangParser::IDENTIFIER);
    setState(187);
    match(TzdLangParser::T__2);
    setState(189);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 33) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
      setState(188);
      paramList();
    }
    setState(191);
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
    setState(193);
    match(TzdLangParser::KW_ENUM);
    setState(194);
    match(TzdLangParser::IDENTIFIER);
    setState(195);
    match(TzdLangParser::T__4);
    setState(197);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::IDENTIFIER) {
      setState(196);
      enumList();
    }
    setState(199);
    match(TzdLangParser::T__5);
   
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
    setState(201);
    match(TzdLangParser::IDENTIFIER);
    setState(206);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(202);
      match(TzdLangParser::T__8);
      setState(203);
      match(TzdLangParser::IDENTIFIER);
      setState(208);
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
    setState(235);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(210);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(209);
        annotationUsage();
      }
      setState(212);
      match(TzdLangParser::KW_CLASS);
      setState(213);
      qualifiedName();
      setState(216);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__6) {
        setState(214);
        match(TzdLangParser::T__6);
        setState(215);
        qualifiedName();
      }
      setState(218);
      match(TzdLangParser::T__4);
      setState(219);
      classBody();
      setState(220);
      match(TzdLangParser::T__5);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(223);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(222);
        annotationUsage();
      }
      setState(225);
      match(TzdLangParser::KW_CLASS);
      setState(226);
      qualifiedName();
      setState(229);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::KW_EXTENDS) {
        setState(227);
        match(TzdLangParser::KW_EXTENDS);
        setState(228);
        qualifiedName();
      }
      setState(231);
      match(TzdLangParser::T__4);
      setState(232);
      classBody();
      setState(233);
      match(TzdLangParser::T__5);
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
    setState(240);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8071872768) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(237);
      classMember();
      setState(242);
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
    setState(251);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(244);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(243);
        annotationUsage();
      }
      setState(247);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 3758096384) != 0)) {
        setState(246);
        accessModifier();
      }
      setState(249);
      memberDecl();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(250);
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
    setState(326);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::KW_VAR: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldVarDeclContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(253);
        match(TzdLangParser::KW_VAR);
        setState(254);
        typeType(0);
        setState(255);
        match(TzdLangParser::IDENTIFIER);
        setState(258);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ASSIGN) {
          setState(256);
          match(TzdLangParser::ASSIGN);
          setState(257);
          expression(0);
        }
        setState(260);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_LET: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldLetDeclContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(262);
        match(TzdLangParser::KW_LET);
        setState(263);
        typeType(0);
        setState(264);
        match(TzdLangParser::IDENTIFIER);
        setState(267);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ASSIGN) {
          setState(265);
          match(TzdLangParser::ASSIGN);
          setState(266);
          expression(0);
        }
        setState(269);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_CONST: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldConstDeclContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(271);
        match(TzdLangParser::KW_CONST);
        setState(272);
        match(TzdLangParser::IDENTIFIER);
        setState(275);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::T__6) {
          setState(273);
          match(TzdLangParser::T__6);
          setState(274);
          typeType(0);
        }
        setState(277);
        match(TzdLangParser::ASSIGN);
        setState(278);
        expression(0);
        setState(279);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_STATIC: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodStaticDeclContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(281);
        match(TzdLangParser::KW_STATIC);
        setState(282);
        match(TzdLangParser::KW_FUN);
        setState(283);
        match(TzdLangParser::IDENTIFIER);
        setState(284);
        match(TzdLangParser::T__2);
        setState(286);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 33) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
          setState(285);
          paramList();
        }
        setState(288);
        match(TzdLangParser::T__3);
        setState(291);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ARROW) {
          setState(289);
          match(TzdLangParser::ARROW);
          setState(290);
          typeType(0);
        }
        setState(293);
        block();
        break;
      }

      case TzdLangParser::KW_ABSTRACT: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodAbstractDeclContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(294);
        match(TzdLangParser::KW_ABSTRACT);
        setState(295);
        match(TzdLangParser::KW_FUN);
        setState(296);
        match(TzdLangParser::IDENTIFIER);
        setState(297);
        match(TzdLangParser::T__2);
        setState(299);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 33) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
          setState(298);
          paramList();
        }
        setState(301);
        match(TzdLangParser::T__3);
        setState(304);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ARROW) {
          setState(302);
          match(TzdLangParser::ARROW);
          setState(303);
          typeType(0);
        }
        setState(306);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_FUN: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodDeclContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(307);
        match(TzdLangParser::KW_FUN);
        setState(308);
        match(TzdLangParser::IDENTIFIER);
        setState(309);
        match(TzdLangParser::T__2);
        setState(311);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 33) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
          setState(310);
          paramList();
        }
        setState(313);
        match(TzdLangParser::T__3);
        setState(316);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ARROW) {
          setState(314);
          match(TzdLangParser::ARROW);
          setState(315);
          typeType(0);
        }
        setState(318);
        block();
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        _localctx = _tracker.createInstance<TzdLangParser::ConstructorDeclContext>(_localctx);
        enterOuterAlt(_localctx, 7);
        setState(319);
        match(TzdLangParser::IDENTIFIER);
        setState(320);
        match(TzdLangParser::T__2);
        setState(322);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 33) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
          setState(321);
          paramList();
        }
        setState(324);
        match(TzdLangParser::T__3);
        setState(325);
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
    setState(328);
    match(TzdLangParser::T__7);
    setState(329);
    match(TzdLangParser::IDENTIFIER);
    setState(335);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__2) {
      setState(330);
      match(TzdLangParser::T__2);
      setState(332);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(331);
        exprList();
      }
      setState(334);
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
    setState(337);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 3758096384) != 0))) {
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
    setState(340);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__7) {
      setState(339);
      annotationUsage();
    }
    setState(342);
    match(TzdLangParser::KW_FUN);
    setState(343);
    match(TzdLangParser::IDENTIFIER);
    setState(344);
    match(TzdLangParser::T__2);
    setState(346);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 33) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
      setState(345);
      paramList();
    }
    setState(348);
    match(TzdLangParser::T__3);
    setState(351);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::ARROW) {
      setState(349);
      match(TzdLangParser::ARROW);
      setState(350);
      typeType(0);
    }
    setState(353);
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
    setState(356);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__7) {
      setState(355);
      annotationUsage();
    }
    setState(358);
    match(TzdLangParser::KW_NATIVE);
    setState(359);
    match(TzdLangParser::KW_FUN);
    setState(360);
    match(TzdLangParser::IDENTIFIER);
    setState(361);
    match(TzdLangParser::T__2);
    setState(363);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 33) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
      setState(362);
      paramList();
    }
    setState(365);
    match(TzdLangParser::T__3);
    setState(368);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::ARROW) {
      setState(366);
      match(TzdLangParser::ARROW);
      setState(367);
      typeType(0);
    }
    setState(370);
    match(TzdLangParser::T__2);
    setState(372);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 142989301053922304) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(371);
      nativeAttrList();
    }
    setState(374);
    match(TzdLangParser::T__3);
    setState(375);
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
    setState(377);
    nativeAttr();
    setState(382);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(378);
      match(TzdLangParser::T__8);
      setState(379);
      nativeAttr();
      setState(384);
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
    setState(387);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::IDENTIFIER: {
        setState(385);
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
        setState(386);
        nativePropKey();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(389);
    match(TzdLangParser::ASSIGN);
    setState(390);
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
    setState(392);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 142989301053922304) != 0))) {
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
    setState(413);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 49, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(394);
      typeType(0);
      setState(395);
      match(TzdLangParser::IDENTIFIER);
      setState(398);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(396);
        match(TzdLangParser::ASSIGN);
        setState(397);
        expression(0);
      }
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(400);
      match(TzdLangParser::KW_VAR);
      setState(401);
      match(TzdLangParser::IDENTIFIER);
      setState(404);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(402);
        match(TzdLangParser::ASSIGN);
        setState(403);
        expression(0);
      }
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(406);
      match(TzdLangParser::IDENTIFIER);
      setState(407);
      match(TzdLangParser::T__6);
      setState(408);
      typeType(0);
      setState(411);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(409);
        match(TzdLangParser::ASSIGN);
        setState(410);
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
    setState(417);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 50, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(415);
      variableDeclaration();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(416);
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
  enterRule(_localctx, 42, TzdLangParser::RuleImportStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(419);
    match(TzdLangParser::KW_IMPORT);
    setState(420);
    match(TzdLangParser::STRING);
    setState(421);
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
  enterRule(_localctx, 44, TzdLangParser::RuleParamList);
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
    setState(423);
    param();
    setState(428);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(424);
      match(TzdLangParser::T__8);
      setState(425);
      param();
      setState(430);
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
  enterRule(_localctx, 46, TzdLangParser::RuleParam);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(439);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 53, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(431);
      typeType(0);
      setState(432);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(434);
      _la = _input->LA(1);
      if (!(((((_la - 33) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 33)) & 4611686018444034049) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(437);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__6) {
        setState(435);
        match(TzdLangParser::T__6);
        setState(436);
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
  enterRule(_localctx, 48, TzdLangParser::RuleBlock);
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
    setState(441);
    match(TzdLangParser::T__4);
    setState(445);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1152351919191171372) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & 246881) != 0)) {
      setState(442);
      statement();
      setState(447);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(448);
    match(TzdLangParser::T__5);
   
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
  size_t startState = 50;
  enterRecursionRule(_localctx, 50, TzdLangParser::RuleExpression, precedence);

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
    setState(461);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 55, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<CastExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(451);
      match(TzdLangParser::T__2);
      setState(452);
      typeType(0);
      setState(453);
      match(TzdLangParser::T__3);
      setState(454);
      expression(19);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<PrefixExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(456);
      _la = _input->LA(1);
      if (!(_la == TzdLangParser::INC

      || _la == TzdLangParser::DEC)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(457);
      expression(15);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<UnaryExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(458);
      _la = _input->LA(1);
      if (!(((((_la - 59) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 59)) & 4701814785) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(459);
      expression(14);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<AtomExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(460);
      atom(0);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(514);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 58, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(512);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 57, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<PowerExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(463);

          if (!(precpred(_ctx, 16))) throw FailedPredicateException(this, "precpred(_ctx, 16)");
          setState(464);
          match(TzdLangParser::POW);
          setState(465);
          expression(16);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<MultiplicativeExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(466);

          if (!(precpred(_ctx, 12))) throw FailedPredicateException(this, "precpred(_ctx, 12)");
          setState(467);
          _la = _input->LA(1);
          if (!(((((_la - 88) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 88)) & 7) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(468);
          expression(13);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<AdditiveExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(469);

          if (!(precpred(_ctx, 11))) throw FailedPredicateException(this, "precpred(_ctx, 11)");
          setState(470);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::PLUS

          || _la == TzdLangParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(471);
          expression(12);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ShiftExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(472);

          if (!(precpred(_ctx, 10))) throw FailedPredicateException(this, "precpred(_ctx, 10)");
          setState(473);
          _la = _input->LA(1);
          if (!(((((_la - 63) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 63)) & 7) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(474);
          expression(11);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<RelationalExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(475);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(476);
          _la = _input->LA(1);
          if (!(((((_la - 76) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 76)) & 196611) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(477);
          expression(10);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<EqualityExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(478);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(479);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::EEQ

          || _la == TzdLangParser::NEQ)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(480);
          expression(9);
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<BitAndExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(481);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(482);
          match(TzdLangParser::BIT_AND);
          setState(483);
          expression(8);
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<BitXorExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(484);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(485);
          match(TzdLangParser::BIT_XOR);
          setState(486);
          expression(7);
          break;
        }

        case 9: {
          auto newContext = _tracker.createInstance<BitOrExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(487);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(488);
          match(TzdLangParser::BIT_OR);
          setState(489);
          expression(6);
          break;
        }

        case 10: {
          auto newContext = _tracker.createInstance<LogicalAndExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(490);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(491);
          match(TzdLangParser::AND);
          setState(492);
          expression(5);
          break;
        }

        case 11: {
          auto newContext = _tracker.createInstance<LogicalOrExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(493);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(494);
          match(TzdLangParser::OR);
          setState(495);
          expression(4);
          break;
        }

        case 12: {
          auto newContext = _tracker.createInstance<AssignmentExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(496);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(497);
          _la = _input->LA(1);
          if (!(((((_la - 60) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 60)) & 17179885511) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(498);
          expression(2);
          break;
        }

        case 13: {
          auto newContext = _tracker.createInstance<IndexExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(499);

          if (!(precpred(_ctx, 18))) throw FailedPredicateException(this, "precpred(_ctx, 18)");
          setState(500);
          match(TzdLangParser::T__12);
          setState(501);
          expression(0);
          setState(502);
          match(TzdLangParser::T__13);
          break;
        }

        case 14: {
          auto newContext = _tracker.createInstance<PostfixExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(504);

          if (!(precpred(_ctx, 17))) throw FailedPredicateException(this, "precpred(_ctx, 17)");
          setState(505);
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
          setState(506);

          if (!(precpred(_ctx, 13))) throw FailedPredicateException(this, "precpred(_ctx, 13)");
          setState(507);
          match(TzdLangParser::KW_IN);
          setState(510);
          _errHandler->sync(this);
          switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 56, _ctx)) {
          case 1: {
            setState(508);
            qualifiedName();
            break;
          }

          case 2: {
            setState(509);
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
      setState(516);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 58, _ctx);
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
  size_t startState = 52;
  enterRecursionRule(_localctx, 52, TzdLangParser::RuleAtom, precedence);

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
    setState(572);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 67, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ParenExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(518);
      match(TzdLangParser::T__2);
      setState(519);
      expression(0);
      setState(520);
      match(TzdLangParser::T__3);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ArrayLiteralExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(522);
      match(TzdLangParser::T__12);
      setState(524);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(523);
        exprList();
      }
      setState(526);
      match(TzdLangParser::T__13);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<MapLiteralExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(527);
      match(TzdLangParser::T__4);
      setState(529);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__2 || ((((_la - 95) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 95)) & 11) != 0)) {
        setState(528);
        mapEntryList();
      }
      setState(531);
      match(TzdLangParser::T__5);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ArrayLiteralExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(532);
      match(TzdLangParser::T__4);
      setState(534);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(533);
        exprList();
      }
      setState(536);
      match(TzdLangParser::T__5);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<IntExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(537);
      match(TzdLangParser::INTEGER);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<FloatExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(538);
      match(TzdLangParser::FLOAT);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<StringExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(539);
      match(TzdLangParser::STRING);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<BoolTrueExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(540);
      match(TzdLangParser::KW_TRUE);
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<BoolFalseExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(541);
      match(TzdLangParser::KW_FALSE);
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<NullExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(542);
      match(TzdLangParser::KW_NULL);
      break;
    }

    case 11: {
      _localctx = _tracker.createInstance<IdExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(543);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<SuperExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(544);
      match(TzdLangParser::KW_SUPER);
      setState(545);
      match(TzdLangParser::T__2);
      setState(547);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(546);
        exprList();
      }
      setState(549);
      match(TzdLangParser::T__3);
      break;
    }

    case 13: {
      _localctx = _tracker.createInstance<PrintFunExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(550);
      printFunction();
      break;
    }

    case 14: {
      _localctx = _tracker.createInstance<NewExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(551);
      match(TzdLangParser::KW_NEW);
      setState(552);
      qualifiedName();
      setState(553);
      match(TzdLangParser::T__2);
      setState(555);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 81)) & 246881) != 0)) {
        setState(554);
        exprList();
      }
      setState(557);
      match(TzdLangParser::T__3);
      setState(559);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 64, _ctx)) {
      case 1: {
        setState(558);
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
      setState(561);
      match(TzdLangParser::KW_FUN);
      setState(562);
      match(TzdLangParser::T__2);
      setState(564);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (((((_la - 33) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 33)) & 4611686018444034049) != 0)) {
        setState(563);
        paramList();
      }
      setState(566);
      match(TzdLangParser::T__3);
      setState(569);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ARROW) {
        setState(567);
        match(TzdLangParser::ARROW);
        setState(568);
        typeType(0);
      }
      setState(571);
      block();
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(585);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 70, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(583);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 69, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<CallExprContext>(_tracker.createInstance<AtomContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleAtom);
          setState(574);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(575);
          match(TzdLangParser::T__2);
          setState(577);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if ((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 81)) & 246881) != 0)) {
            setState(576);
            exprList();
          }
          setState(579);
          match(TzdLangParser::T__3);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<MemberAccessExprContext>(_tracker.createInstance<AtomContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleAtom);
          setState(580);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(581);
          match(TzdLangParser::T__0);
          setState(582);
          match(TzdLangParser::IDENTIFIER);
          break;
        }

        default:
          break;
        } 
      }
      setState(587);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 70, _ctx);
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
  enterRule(_localctx, 54, TzdLangParser::RuleClassOverrideBlock);
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
    setState(588);
    match(TzdLangParser::T__4);
    setState(592);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8071872768) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(589);
      classMember();
      setState(594);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(595);
    match(TzdLangParser::T__5);
   
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
  enterRule(_localctx, 56, TzdLangParser::RulePrintFunction);
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
    setState(597);
    match(TzdLangParser::KW_PRINT);
    setState(598);
    match(TzdLangParser::T__2);
    setState(600);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1008938262296797224) != 0) || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & 246881) != 0)) {
      setState(599);
      exprList();
    }
    setState(602);
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
  enterRule(_localctx, 58, TzdLangParser::RuleExprList);
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
    setState(604);
    expression(0);
    setState(609);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(605);
      match(TzdLangParser::T__8);
      setState(606);
      expression(0);
      setState(611);
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
  enterRule(_localctx, 60, TzdLangParser::RuleMapEntryList);
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
    setState(612);
    mapEntry();
    setState(617);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 74, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(613);
        match(TzdLangParser::T__8);
        setState(614);
        mapEntry(); 
      }
      setState(619);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 74, _ctx);
    }
    setState(621);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__8) {
      setState(620);
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
  enterRule(_localctx, 62, TzdLangParser::RuleMapEntry);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(623);
    mapKey();
    setState(624);
    match(TzdLangParser::T__6);
    setState(625);
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
  enterRule(_localctx, 64, TzdLangParser::RuleMapKey);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(634);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::STRING: {
        enterOuterAlt(_localctx, 1);
        setState(627);
        match(TzdLangParser::STRING);
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 2);
        setState(628);
        match(TzdLangParser::IDENTIFIER);
        break;
      }

      case TzdLangParser::INTEGER: {
        enterOuterAlt(_localctx, 3);
        setState(629);
        match(TzdLangParser::INTEGER);
        break;
      }

      case TzdLangParser::T__2: {
        enterOuterAlt(_localctx, 4);
        setState(630);
        match(TzdLangParser::T__2);
        setState(631);
        expression(0);
        setState(632);
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
  size_t startState = 66;
  enterRecursionRule(_localctx, 66, TzdLangParser::RuleTypeType, precedence);

    

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
    setState(645);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::T_INT: {
        setState(637);
        match(TzdLangParser::T_INT);
        break;
      }

      case TzdLangParser::T_FLOAT: {
        setState(638);
        match(TzdLangParser::T_FLOAT);
        break;
      }

      case TzdLangParser::T_STRING: {
        setState(639);
        match(TzdLangParser::T_STRING);
        break;
      }

      case TzdLangParser::T_BOOL: {
        setState(640);
        match(TzdLangParser::T_BOOL);
        break;
      }

      case TzdLangParser::T_VOID: {
        setState(641);
        match(TzdLangParser::T_VOID);
        break;
      }

      case TzdLangParser::T_PTR: {
        setState(642);
        match(TzdLangParser::T_PTR);
        break;
      }

      case TzdLangParser::T_FUNCTION: {
        setState(643);
        match(TzdLangParser::T_FUNCTION);
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        setState(644);
        qualifiedName();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(651);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 78, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        _localctx = _tracker.createInstance<TypeTypeContext>(parentContext, parentState);
        pushNewRecursionContext(_localctx, startState, RuleTypeType);
        setState(647);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(648);
        match(TzdLangParser::T__14); 
      }
      setState(653);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 78, _ctx);
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
    case 25: return expressionSempred(antlrcpp::downCast<ExpressionContext *>(context), predicateIndex);
    case 26: return atomSempred(antlrcpp::downCast<AtomContext *>(context), predicateIndex);
    case 33: return typeTypeSempred(antlrcpp::downCast<TypeTypeContext *>(context), predicateIndex);

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
