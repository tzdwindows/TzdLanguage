
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
      "printFunction", "exprList", "typeType"
    },
    std::vector<std::string>{
      "", "'.'", "';'", "'('", "')'", "'{'", "'}'", "':'", "'@'", "','", 
      "'type'", "'dll'", "'prototype'", "'['", "']'", "'^'", "'[]'", "'var'", 
      "'const'", "'let'", "'static'", "'abstract'", "'enum'", "'in'", "'super'", 
      "'native'", "'import'", "", "'class'", "'extends'", "'public'", "'private'", 
      "'protected'", "'fun'", "", "'if'", "'else'", "'while'", "'for'", 
      "'break'", "'continue'", "'switch'", "'case'", "'default'", "'new'", 
      "'true'", "'false'", "'null'", "'throw'", "'try'", "'catch'", "'int'", 
      "'float'", "'string'", "'bool'", "'void'", "", "", "'++'", "'--'", 
      "'gxxx'", "'+'", "'-'", "'*'", "'/'", "'%'", "'!'", "'>='", "'<='", 
      "'>'", "'<'", "'=='", "'!='", "'&&'", "'||'", "'='", "'+='", "'-='", 
      "'*='", "'/='"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "KW_VAR", "KW_CONST", "KW_LET", "KW_STATIC", "KW_ABSTRACT", "KW_ENUM", 
      "KW_IN", "KW_SUPER", "KW_NATIVE", "KW_IMPORT", "KW_PRINT", "KW_CLASS", 
      "KW_EXTENDS", "KW_PUBLIC", "KW_PRIVATE", "KW_PROTECTED", "KW_FUN", 
      "KW_RET", "KW_IF", "KW_ELSE", "KW_WHILE", "KW_FOR", "KW_BREAK", "KW_CONTINUE", 
      "KW_SWITCH", "KW_CASE", "KW_DEFAULT", "KW_NEW", "KW_TRUE", "KW_FALSE", 
      "KW_NULL", "KW_THROW", "KW_TRY", "KW_CATCH", "T_INT", "T_FLOAT", "T_STRING", 
      "T_BOOL", "T_VOID", "T_PTR", "T_FUNCTION", "INC", "DEC", "GXXX", "PLUS", 
      "MINUS", "MUL", "DIV", "MOD", "NOT", "GE", "LE", "GT", "LT", "EEQ", 
      "NEQ", "AND", "OR", "ASSIGN", "PLUS_ASSIGN", "MIN_ASSIGN", "MUL_ASSIGN", 
      "DIV_ASSIGN", "IDENTIFIER", "INTEGER", "FLOAT", "STRING", "LINE_COMMENT", 
      "BLOCK_COMMENT", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,86,584,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,1,0,5,0,64,8,0,10,0,12,0,67,9,0,1,0,1,0,1,1,1,
  	1,1,1,5,1,74,8,1,10,1,12,1,77,9,1,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
  	1,2,1,2,1,2,3,2,91,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,101,8,2,1,
  	2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,112,8,2,1,2,1,2,3,2,116,8,2,1,2,
  	1,2,3,2,120,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,5,2,134,
  	8,2,10,2,12,2,137,9,2,1,2,3,2,140,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
  	1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,160,8,2,1,3,1,3,1,3,1,3,5,
  	3,166,8,3,10,3,12,3,169,9,3,1,4,1,4,1,4,5,4,174,8,4,10,4,12,4,177,9,4,
  	1,5,1,5,1,5,1,5,1,5,3,5,184,8,5,1,5,1,5,1,6,1,6,1,6,1,6,3,6,192,8,6,1,
  	6,1,6,1,7,1,7,1,7,5,7,199,8,7,10,7,12,7,202,9,7,1,8,3,8,205,8,8,1,8,1,
  	8,1,8,1,8,3,8,211,8,8,1,8,1,8,1,8,1,8,1,8,3,8,218,8,8,1,8,1,8,1,8,1,8,
  	3,8,224,8,8,1,8,1,8,1,8,1,8,3,8,230,8,8,1,9,5,9,233,8,9,10,9,12,9,236,
  	9,9,1,10,3,10,239,8,10,1,10,3,10,242,8,10,1,10,1,10,3,10,246,8,10,1,11,
  	1,11,1,11,1,11,1,11,3,11,253,8,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,
  	3,11,262,8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,270,8,11,1,11,1,11,1,
  	11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,281,8,11,1,11,1,11,1,11,1,11,1,
  	11,1,11,1,11,3,11,290,8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,298,8,11,
  	1,11,1,11,1,11,1,11,1,11,3,11,305,8,11,1,11,1,11,3,11,309,8,11,1,12,1,
  	12,1,12,1,12,3,12,315,8,12,1,12,3,12,318,8,12,1,13,1,13,1,14,3,14,323,
  	8,14,1,14,1,14,1,14,1,14,3,14,329,8,14,1,14,1,14,1,14,1,15,3,15,335,8,
  	15,1,15,1,15,1,15,1,15,1,15,3,15,342,8,15,1,15,1,15,1,15,3,15,347,8,15,
  	1,15,1,15,1,15,1,16,1,16,1,16,5,16,355,8,16,10,16,12,16,358,9,16,1,17,
  	1,17,3,17,362,8,17,1,17,1,17,1,17,1,18,1,18,1,19,1,19,1,19,1,19,3,19,
  	373,8,19,1,19,1,19,1,19,1,19,3,19,379,8,19,1,19,1,19,1,19,1,19,1,19,3,
  	19,386,8,19,3,19,388,8,19,1,20,1,20,3,20,392,8,20,1,21,1,21,1,21,1,21,
  	1,22,1,22,1,22,5,22,401,8,22,10,22,12,22,404,9,22,1,23,1,23,1,23,1,23,
  	1,23,1,23,3,23,412,8,23,3,23,414,8,23,1,24,1,24,5,24,418,8,24,10,24,12,
  	24,421,9,24,1,24,1,24,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
  	25,1,25,3,25,436,8,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
  	25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
  	25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,3,25,473,8,
  	25,5,25,475,8,25,10,25,12,25,478,9,25,1,26,1,26,1,26,1,26,1,26,1,26,1,
  	26,3,26,487,8,26,1,26,1,26,1,26,3,26,492,8,26,1,26,1,26,1,26,1,26,1,26,
  	1,26,1,26,1,26,1,26,1,26,1,26,3,26,505,8,26,1,26,1,26,1,26,1,26,1,26,
  	1,26,3,26,513,8,26,1,26,1,26,3,26,517,8,26,1,26,1,26,1,26,3,26,522,8,
  	26,1,26,1,26,3,26,526,8,26,1,26,1,26,1,26,3,26,531,8,26,1,26,1,26,1,26,
  	1,26,5,26,537,8,26,10,26,12,26,540,9,26,1,27,1,27,5,27,544,8,27,10,27,
  	12,27,547,9,27,1,27,1,27,1,28,1,28,1,28,3,28,554,8,28,1,28,1,28,1,29,
  	1,29,1,29,5,29,561,8,29,10,29,12,29,564,9,29,1,30,1,30,1,30,1,30,1,30,
  	1,30,1,30,1,30,1,30,3,30,575,8,30,1,30,1,30,5,30,579,8,30,10,30,12,30,
  	582,9,30,1,30,0,3,50,52,60,31,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,
  	30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,0,11,1,0,30,32,2,0,81,
  	81,83,83,3,0,10,12,33,34,51,57,3,0,34,34,51,57,80,80,1,0,58,59,3,0,60,
  	60,62,62,66,66,1,0,63,65,1,0,61,62,1,0,67,70,1,0,71,72,1,0,75,79,673,
  	0,65,1,0,0,0,2,70,1,0,0,0,4,159,1,0,0,0,6,161,1,0,0,0,8,170,1,0,0,0,10,
  	178,1,0,0,0,12,187,1,0,0,0,14,195,1,0,0,0,16,229,1,0,0,0,18,234,1,0,0,
  	0,20,245,1,0,0,0,22,308,1,0,0,0,24,310,1,0,0,0,26,319,1,0,0,0,28,322,
  	1,0,0,0,30,334,1,0,0,0,32,351,1,0,0,0,34,361,1,0,0,0,36,366,1,0,0,0,38,
  	387,1,0,0,0,40,391,1,0,0,0,42,393,1,0,0,0,44,397,1,0,0,0,46,413,1,0,0,
  	0,48,415,1,0,0,0,50,435,1,0,0,0,52,525,1,0,0,0,54,541,1,0,0,0,56,550,
  	1,0,0,0,58,557,1,0,0,0,60,574,1,0,0,0,62,64,3,4,2,0,63,62,1,0,0,0,64,
  	67,1,0,0,0,65,63,1,0,0,0,65,66,1,0,0,0,66,68,1,0,0,0,67,65,1,0,0,0,68,
  	69,5,0,0,1,69,1,1,0,0,0,70,75,5,80,0,0,71,72,5,1,0,0,72,74,5,80,0,0,73,
  	71,1,0,0,0,74,77,1,0,0,0,75,73,1,0,0,0,75,76,1,0,0,0,76,3,1,0,0,0,77,
  	75,1,0,0,0,78,160,3,48,24,0,79,160,3,16,8,0,80,160,3,10,5,0,81,160,3,
  	12,6,0,82,160,3,28,14,0,83,160,3,30,15,0,84,85,3,38,19,0,85,86,5,2,0,
  	0,86,160,1,0,0,0,87,160,3,42,21,0,88,90,5,34,0,0,89,91,3,50,25,0,90,89,
  	1,0,0,0,90,91,1,0,0,0,91,92,1,0,0,0,92,160,5,2,0,0,93,94,5,35,0,0,94,
  	95,5,3,0,0,95,96,3,50,25,0,96,97,5,4,0,0,97,100,3,4,2,0,98,99,5,36,0,
  	0,99,101,3,4,2,0,100,98,1,0,0,0,100,101,1,0,0,0,101,160,1,0,0,0,102,103,
  	5,37,0,0,103,104,5,3,0,0,104,105,3,50,25,0,105,106,5,4,0,0,106,107,3,
  	4,2,0,107,160,1,0,0,0,108,109,5,38,0,0,109,111,5,3,0,0,110,112,3,40,20,
  	0,111,110,1,0,0,0,111,112,1,0,0,0,112,113,1,0,0,0,113,115,5,2,0,0,114,
  	116,3,50,25,0,115,114,1,0,0,0,115,116,1,0,0,0,116,117,1,0,0,0,117,119,
  	5,2,0,0,118,120,3,50,25,0,119,118,1,0,0,0,119,120,1,0,0,0,120,121,1,0,
  	0,0,121,122,5,4,0,0,122,160,3,4,2,0,123,124,5,39,0,0,124,160,5,2,0,0,
  	125,126,5,40,0,0,126,160,5,2,0,0,127,128,5,41,0,0,128,129,5,3,0,0,129,
  	130,3,50,25,0,130,131,5,4,0,0,131,135,5,5,0,0,132,134,3,6,3,0,133,132,
  	1,0,0,0,134,137,1,0,0,0,135,133,1,0,0,0,135,136,1,0,0,0,136,139,1,0,0,
  	0,137,135,1,0,0,0,138,140,3,8,4,0,139,138,1,0,0,0,139,140,1,0,0,0,140,
  	141,1,0,0,0,141,142,5,6,0,0,142,160,1,0,0,0,143,144,5,49,0,0,144,145,
  	3,48,24,0,145,146,5,50,0,0,146,147,5,3,0,0,147,148,5,80,0,0,148,149,5,
  	4,0,0,149,150,3,48,24,0,150,160,1,0,0,0,151,152,5,48,0,0,152,153,3,50,
  	25,0,153,154,5,2,0,0,154,160,1,0,0,0,155,156,3,50,25,0,156,157,5,2,0,
  	0,157,160,1,0,0,0,158,160,5,2,0,0,159,78,1,0,0,0,159,79,1,0,0,0,159,80,
  	1,0,0,0,159,81,1,0,0,0,159,82,1,0,0,0,159,83,1,0,0,0,159,84,1,0,0,0,159,
  	87,1,0,0,0,159,88,1,0,0,0,159,93,1,0,0,0,159,102,1,0,0,0,159,108,1,0,
  	0,0,159,123,1,0,0,0,159,125,1,0,0,0,159,127,1,0,0,0,159,143,1,0,0,0,159,
  	151,1,0,0,0,159,155,1,0,0,0,159,158,1,0,0,0,160,5,1,0,0,0,161,162,5,42,
  	0,0,162,163,3,50,25,0,163,167,5,7,0,0,164,166,3,4,2,0,165,164,1,0,0,0,
  	166,169,1,0,0,0,167,165,1,0,0,0,167,168,1,0,0,0,168,7,1,0,0,0,169,167,
  	1,0,0,0,170,171,5,43,0,0,171,175,5,7,0,0,172,174,3,4,2,0,173,172,1,0,
  	0,0,174,177,1,0,0,0,175,173,1,0,0,0,175,176,1,0,0,0,176,9,1,0,0,0,177,
  	175,1,0,0,0,178,179,5,28,0,0,179,180,5,8,0,0,180,181,5,80,0,0,181,183,
  	5,3,0,0,182,184,3,44,22,0,183,182,1,0,0,0,183,184,1,0,0,0,184,185,1,0,
  	0,0,185,186,5,4,0,0,186,11,1,0,0,0,187,188,5,22,0,0,188,189,5,80,0,0,
  	189,191,5,5,0,0,190,192,3,14,7,0,191,190,1,0,0,0,191,192,1,0,0,0,192,
  	193,1,0,0,0,193,194,5,6,0,0,194,13,1,0,0,0,195,200,5,80,0,0,196,197,5,
  	9,0,0,197,199,5,80,0,0,198,196,1,0,0,0,199,202,1,0,0,0,200,198,1,0,0,
  	0,200,201,1,0,0,0,201,15,1,0,0,0,202,200,1,0,0,0,203,205,3,24,12,0,204,
  	203,1,0,0,0,204,205,1,0,0,0,205,206,1,0,0,0,206,207,5,28,0,0,207,210,
  	3,2,1,0,208,209,5,7,0,0,209,211,3,2,1,0,210,208,1,0,0,0,210,211,1,0,0,
  	0,211,212,1,0,0,0,212,213,5,5,0,0,213,214,3,18,9,0,214,215,5,6,0,0,215,
  	230,1,0,0,0,216,218,3,24,12,0,217,216,1,0,0,0,217,218,1,0,0,0,218,219,
  	1,0,0,0,219,220,5,28,0,0,220,223,3,2,1,0,221,222,5,29,0,0,222,224,3,2,
  	1,0,223,221,1,0,0,0,223,224,1,0,0,0,224,225,1,0,0,0,225,226,5,5,0,0,226,
  	227,3,18,9,0,227,228,5,6,0,0,228,230,1,0,0,0,229,204,1,0,0,0,229,217,
  	1,0,0,0,230,17,1,0,0,0,231,233,3,20,10,0,232,231,1,0,0,0,233,236,1,0,
  	0,0,234,232,1,0,0,0,234,235,1,0,0,0,235,19,1,0,0,0,236,234,1,0,0,0,237,
  	239,3,24,12,0,238,237,1,0,0,0,238,239,1,0,0,0,239,241,1,0,0,0,240,242,
  	3,26,13,0,241,240,1,0,0,0,241,242,1,0,0,0,242,243,1,0,0,0,243,246,3,22,
  	11,0,244,246,3,30,15,0,245,238,1,0,0,0,245,244,1,0,0,0,246,21,1,0,0,0,
  	247,248,5,17,0,0,248,249,3,60,30,0,249,252,5,80,0,0,250,251,5,75,0,0,
  	251,253,3,50,25,0,252,250,1,0,0,0,252,253,1,0,0,0,253,254,1,0,0,0,254,
  	255,5,2,0,0,255,309,1,0,0,0,256,257,5,19,0,0,257,258,3,60,30,0,258,261,
  	5,80,0,0,259,260,5,75,0,0,260,262,3,50,25,0,261,259,1,0,0,0,261,262,1,
  	0,0,0,262,263,1,0,0,0,263,264,5,2,0,0,264,309,1,0,0,0,265,266,5,18,0,
  	0,266,269,5,80,0,0,267,268,5,7,0,0,268,270,3,60,30,0,269,267,1,0,0,0,
  	269,270,1,0,0,0,270,271,1,0,0,0,271,272,5,75,0,0,272,273,3,50,25,0,273,
  	274,5,2,0,0,274,309,1,0,0,0,275,276,5,20,0,0,276,277,5,33,0,0,277,278,
  	5,80,0,0,278,280,5,3,0,0,279,281,3,44,22,0,280,279,1,0,0,0,280,281,1,
  	0,0,0,281,282,1,0,0,0,282,283,5,4,0,0,283,309,3,48,24,0,284,285,5,21,
  	0,0,285,286,5,33,0,0,286,287,5,80,0,0,287,289,5,3,0,0,288,290,3,44,22,
  	0,289,288,1,0,0,0,289,290,1,0,0,0,290,291,1,0,0,0,291,292,5,4,0,0,292,
  	309,5,2,0,0,293,294,5,33,0,0,294,295,5,80,0,0,295,297,5,3,0,0,296,298,
  	3,44,22,0,297,296,1,0,0,0,297,298,1,0,0,0,298,299,1,0,0,0,299,300,5,4,
  	0,0,300,309,3,48,24,0,301,302,5,80,0,0,302,304,5,3,0,0,303,305,3,44,22,
  	0,304,303,1,0,0,0,304,305,1,0,0,0,305,306,1,0,0,0,306,307,5,4,0,0,307,
  	309,3,48,24,0,308,247,1,0,0,0,308,256,1,0,0,0,308,265,1,0,0,0,308,275,
  	1,0,0,0,308,284,1,0,0,0,308,293,1,0,0,0,308,301,1,0,0,0,309,23,1,0,0,
  	0,310,311,5,8,0,0,311,317,5,80,0,0,312,314,5,3,0,0,313,315,3,58,29,0,
  	314,313,1,0,0,0,314,315,1,0,0,0,315,316,1,0,0,0,316,318,5,4,0,0,317,312,
  	1,0,0,0,317,318,1,0,0,0,318,25,1,0,0,0,319,320,7,0,0,0,320,27,1,0,0,0,
  	321,323,3,24,12,0,322,321,1,0,0,0,322,323,1,0,0,0,323,324,1,0,0,0,324,
  	325,5,33,0,0,325,326,5,80,0,0,326,328,5,3,0,0,327,329,3,44,22,0,328,327,
  	1,0,0,0,328,329,1,0,0,0,329,330,1,0,0,0,330,331,5,4,0,0,331,332,3,48,
  	24,0,332,29,1,0,0,0,333,335,3,24,12,0,334,333,1,0,0,0,334,335,1,0,0,0,
  	335,336,1,0,0,0,336,337,5,25,0,0,337,338,5,33,0,0,338,339,5,80,0,0,339,
  	341,5,3,0,0,340,342,3,44,22,0,341,340,1,0,0,0,341,342,1,0,0,0,342,343,
  	1,0,0,0,343,344,5,4,0,0,344,346,5,3,0,0,345,347,3,32,16,0,346,345,1,0,
  	0,0,346,347,1,0,0,0,347,348,1,0,0,0,348,349,5,4,0,0,349,350,5,2,0,0,350,
  	31,1,0,0,0,351,356,3,34,17,0,352,353,5,9,0,0,353,355,3,34,17,0,354,352,
  	1,0,0,0,355,358,1,0,0,0,356,354,1,0,0,0,356,357,1,0,0,0,357,33,1,0,0,
  	0,358,356,1,0,0,0,359,362,5,80,0,0,360,362,3,36,18,0,361,359,1,0,0,0,
  	361,360,1,0,0,0,362,363,1,0,0,0,363,364,5,75,0,0,364,365,7,1,0,0,365,
  	35,1,0,0,0,366,367,7,2,0,0,367,37,1,0,0,0,368,369,3,60,30,0,369,372,5,
  	80,0,0,370,371,5,75,0,0,371,373,3,50,25,0,372,370,1,0,0,0,372,373,1,0,
  	0,0,373,388,1,0,0,0,374,375,5,17,0,0,375,378,5,80,0,0,376,377,5,75,0,
  	0,377,379,3,50,25,0,378,376,1,0,0,0,378,379,1,0,0,0,379,388,1,0,0,0,380,
  	381,5,80,0,0,381,382,5,7,0,0,382,385,3,60,30,0,383,384,5,75,0,0,384,386,
  	3,50,25,0,385,383,1,0,0,0,385,386,1,0,0,0,386,388,1,0,0,0,387,368,1,0,
  	0,0,387,374,1,0,0,0,387,380,1,0,0,0,388,39,1,0,0,0,389,392,3,38,19,0,
  	390,392,3,50,25,0,391,389,1,0,0,0,391,390,1,0,0,0,392,41,1,0,0,0,393,
  	394,5,26,0,0,394,395,5,83,0,0,395,396,5,2,0,0,396,43,1,0,0,0,397,402,
  	3,46,23,0,398,399,5,9,0,0,399,401,3,46,23,0,400,398,1,0,0,0,401,404,1,
  	0,0,0,402,400,1,0,0,0,402,403,1,0,0,0,403,45,1,0,0,0,404,402,1,0,0,0,
  	405,406,3,60,30,0,406,407,5,80,0,0,407,414,1,0,0,0,408,411,7,3,0,0,409,
  	410,5,7,0,0,410,412,3,60,30,0,411,409,1,0,0,0,411,412,1,0,0,0,412,414,
  	1,0,0,0,413,405,1,0,0,0,413,408,1,0,0,0,414,47,1,0,0,0,415,419,5,5,0,
  	0,416,418,3,4,2,0,417,416,1,0,0,0,418,421,1,0,0,0,419,417,1,0,0,0,419,
  	420,1,0,0,0,420,422,1,0,0,0,421,419,1,0,0,0,422,423,5,6,0,0,423,49,1,
  	0,0,0,424,425,6,25,-1,0,425,426,5,3,0,0,426,427,3,60,30,0,427,428,5,4,
  	0,0,428,429,3,50,25,15,429,436,1,0,0,0,430,431,7,4,0,0,431,436,3,50,25,
  	11,432,433,7,5,0,0,433,436,3,50,25,10,434,436,3,52,26,0,435,424,1,0,0,
  	0,435,430,1,0,0,0,435,432,1,0,0,0,435,434,1,0,0,0,436,476,1,0,0,0,437,
  	438,10,12,0,0,438,439,5,15,0,0,439,475,3,50,25,13,440,441,10,8,0,0,441,
  	442,7,6,0,0,442,475,3,50,25,9,443,444,10,7,0,0,444,445,7,7,0,0,445,475,
  	3,50,25,8,446,447,10,6,0,0,447,448,7,8,0,0,448,475,3,50,25,7,449,450,
  	10,5,0,0,450,451,7,9,0,0,451,475,3,50,25,6,452,453,10,4,0,0,453,454,5,
  	73,0,0,454,475,3,50,25,5,455,456,10,3,0,0,456,457,5,74,0,0,457,475,3,
  	50,25,4,458,459,10,2,0,0,459,460,7,10,0,0,460,475,3,50,25,2,461,462,10,
  	14,0,0,462,463,5,13,0,0,463,464,3,50,25,0,464,465,5,14,0,0,465,475,1,
  	0,0,0,466,467,10,13,0,0,467,475,7,4,0,0,468,469,10,9,0,0,469,472,5,23,
  	0,0,470,473,3,2,1,0,471,473,3,60,30,0,472,470,1,0,0,0,472,471,1,0,0,0,
  	473,475,1,0,0,0,474,437,1,0,0,0,474,440,1,0,0,0,474,443,1,0,0,0,474,446,
  	1,0,0,0,474,449,1,0,0,0,474,452,1,0,0,0,474,455,1,0,0,0,474,458,1,0,0,
  	0,474,461,1,0,0,0,474,466,1,0,0,0,474,468,1,0,0,0,475,478,1,0,0,0,476,
  	474,1,0,0,0,476,477,1,0,0,0,477,51,1,0,0,0,478,476,1,0,0,0,479,480,6,
  	26,-1,0,480,481,5,3,0,0,481,482,3,50,25,0,482,483,5,4,0,0,483,526,1,0,
  	0,0,484,486,5,13,0,0,485,487,3,58,29,0,486,485,1,0,0,0,486,487,1,0,0,
  	0,487,488,1,0,0,0,488,526,5,14,0,0,489,491,5,5,0,0,490,492,3,58,29,0,
  	491,490,1,0,0,0,491,492,1,0,0,0,492,493,1,0,0,0,493,526,5,6,0,0,494,526,
  	5,81,0,0,495,526,5,82,0,0,496,526,5,83,0,0,497,526,5,45,0,0,498,526,5,
  	46,0,0,499,526,5,47,0,0,500,526,5,80,0,0,501,502,5,24,0,0,502,504,5,3,
  	0,0,503,505,3,58,29,0,504,503,1,0,0,0,504,505,1,0,0,0,505,506,1,0,0,0,
  	506,526,5,4,0,0,507,526,3,56,28,0,508,509,5,44,0,0,509,510,3,2,1,0,510,
  	512,5,3,0,0,511,513,3,58,29,0,512,511,1,0,0,0,512,513,1,0,0,0,513,514,
  	1,0,0,0,514,516,5,4,0,0,515,517,3,54,27,0,516,515,1,0,0,0,516,517,1,0,
  	0,0,517,526,1,0,0,0,518,519,5,33,0,0,519,521,5,3,0,0,520,522,3,44,22,
  	0,521,520,1,0,0,0,521,522,1,0,0,0,522,523,1,0,0,0,523,524,5,4,0,0,524,
  	526,3,48,24,0,525,479,1,0,0,0,525,484,1,0,0,0,525,489,1,0,0,0,525,494,
  	1,0,0,0,525,495,1,0,0,0,525,496,1,0,0,0,525,497,1,0,0,0,525,498,1,0,0,
  	0,525,499,1,0,0,0,525,500,1,0,0,0,525,501,1,0,0,0,525,507,1,0,0,0,525,
  	508,1,0,0,0,525,518,1,0,0,0,526,538,1,0,0,0,527,528,10,4,0,0,528,530,
  	5,3,0,0,529,531,3,58,29,0,530,529,1,0,0,0,530,531,1,0,0,0,531,532,1,0,
  	0,0,532,537,5,4,0,0,533,534,10,3,0,0,534,535,5,1,0,0,535,537,5,80,0,0,
  	536,527,1,0,0,0,536,533,1,0,0,0,537,540,1,0,0,0,538,536,1,0,0,0,538,539,
  	1,0,0,0,539,53,1,0,0,0,540,538,1,0,0,0,541,545,5,5,0,0,542,544,3,20,10,
  	0,543,542,1,0,0,0,544,547,1,0,0,0,545,543,1,0,0,0,545,546,1,0,0,0,546,
  	548,1,0,0,0,547,545,1,0,0,0,548,549,5,6,0,0,549,55,1,0,0,0,550,551,5,
  	27,0,0,551,553,5,3,0,0,552,554,3,58,29,0,553,552,1,0,0,0,553,554,1,0,
  	0,0,554,555,1,0,0,0,555,556,5,4,0,0,556,57,1,0,0,0,557,562,3,50,25,0,
  	558,559,5,9,0,0,559,561,3,50,25,0,560,558,1,0,0,0,561,564,1,0,0,0,562,
  	560,1,0,0,0,562,563,1,0,0,0,563,59,1,0,0,0,564,562,1,0,0,0,565,566,6,
  	30,-1,0,566,575,5,51,0,0,567,575,5,52,0,0,568,575,5,53,0,0,569,575,5,
  	54,0,0,570,575,5,55,0,0,571,575,5,56,0,0,572,575,5,57,0,0,573,575,3,2,
  	1,0,574,565,1,0,0,0,574,567,1,0,0,0,574,568,1,0,0,0,574,569,1,0,0,0,574,
  	570,1,0,0,0,574,571,1,0,0,0,574,572,1,0,0,0,574,573,1,0,0,0,575,580,1,
  	0,0,0,576,577,10,1,0,0,577,579,5,16,0,0,578,576,1,0,0,0,579,582,1,0,0,
  	0,580,578,1,0,0,0,580,581,1,0,0,0,581,61,1,0,0,0,582,580,1,0,0,0,69,65,
  	75,90,100,111,115,119,135,139,159,167,175,183,191,200,204,210,217,223,
  	229,234,238,241,245,252,261,269,280,289,297,304,308,314,317,322,328,334,
  	341,346,356,361,372,378,385,387,391,402,411,413,419,435,472,474,476,486,
  	491,504,512,516,521,525,530,536,538,545,553,562,574,580
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
    setState(65);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 6916389856809722156) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 66)) & 245761) != 0)) {
      setState(62);
      statement();
      setState(67);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(68);
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
    setState(70);
    match(TzdLangParser::IDENTIFIER);
    setState(75);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(71);
        match(TzdLangParser::T__0);
        setState(72);
        match(TzdLangParser::IDENTIFIER); 
      }
      setState(77);
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
    setState(159);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<TzdLangParser::BlockStmtContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(78);
      block();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<TzdLangParser::ClassDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(79);
      classDeclaration();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<TzdLangParser::AnnotationDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(80);
      annotationDeclaration();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<TzdLangParser::EnumDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(81);
      enumDeclaration();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<TzdLangParser::FunDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(82);
      functionDeclaration();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<TzdLangParser::NativeFunDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(83);
      nativeFunctionDeclaration();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<TzdLangParser::VarDeclStmtContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(84);
      variableDeclaration();
      setState(85);
      match(TzdLangParser::T__1);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<TzdLangParser::ImportStmtContext>(_localctx);
      enterOuterAlt(_localctx, 8);
      setState(87);
      importStatement();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<TzdLangParser::ReturnStmtContext>(_localctx);
      enterOuterAlt(_localctx, 9);
      setState(88);
      match(TzdLangParser::KW_RET);
      setState(90);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 66)) & 245761) != 0)) {
        setState(89);
        expression(0);
      }
      setState(92);
      match(TzdLangParser::T__1);
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<TzdLangParser::IfStmtContext>(_localctx);
      enterOuterAlt(_localctx, 10);
      setState(93);
      match(TzdLangParser::KW_IF);
      setState(94);
      match(TzdLangParser::T__2);
      setState(95);
      expression(0);
      setState(96);
      match(TzdLangParser::T__3);
      setState(97);
      statement();
      setState(100);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
      case 1: {
        setState(98);
        match(TzdLangParser::KW_ELSE);
        setState(99);
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
      setState(102);
      match(TzdLangParser::KW_WHILE);
      setState(103);
      match(TzdLangParser::T__2);
      setState(104);
      expression(0);
      setState(105);
      match(TzdLangParser::T__3);
      setState(106);
      statement();
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<TzdLangParser::ForStmtContext>(_localctx);
      enterOuterAlt(_localctx, 12);
      setState(108);
      match(TzdLangParser::KW_FOR);
      setState(109);
      match(TzdLangParser::T__2);
      setState(111);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 6915541119359131688) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 66)) & 245761) != 0)) {
        setState(110);
        forInit();
      }
      setState(113);
      match(TzdLangParser::T__1);
      setState(115);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 66)) & 245761) != 0)) {
        setState(114);
        antlrcpp::downCast<ForStmtContext *>(_localctx)->cond = expression(0);
      }
      setState(117);
      match(TzdLangParser::T__1);
      setState(119);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 66)) & 245761) != 0)) {
        setState(118);
        antlrcpp::downCast<ForStmtContext *>(_localctx)->step = expression(0);
      }
      setState(121);
      match(TzdLangParser::T__3);
      setState(122);
      statement();
      break;
    }

    case 13: {
      _localctx = _tracker.createInstance<TzdLangParser::BreakStmtContext>(_localctx);
      enterOuterAlt(_localctx, 13);
      setState(123);
      match(TzdLangParser::KW_BREAK);
      setState(124);
      match(TzdLangParser::T__1);
      break;
    }

    case 14: {
      _localctx = _tracker.createInstance<TzdLangParser::ContinueStmtContext>(_localctx);
      enterOuterAlt(_localctx, 14);
      setState(125);
      match(TzdLangParser::KW_CONTINUE);
      setState(126);
      match(TzdLangParser::T__1);
      break;
    }

    case 15: {
      _localctx = _tracker.createInstance<TzdLangParser::SwitchStmtContext>(_localctx);
      enterOuterAlt(_localctx, 15);
      setState(127);
      match(TzdLangParser::KW_SWITCH);
      setState(128);
      match(TzdLangParser::T__2);
      setState(129);
      expression(0);
      setState(130);
      match(TzdLangParser::T__3);
      setState(131);
      match(TzdLangParser::T__4);
      setState(135);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == TzdLangParser::KW_CASE) {
        setState(132);
        switchCase();
        setState(137);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(139);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::KW_DEFAULT) {
        setState(138);
        switchDefault();
      }
      setState(141);
      match(TzdLangParser::T__5);
      break;
    }

    case 16: {
      _localctx = _tracker.createInstance<TzdLangParser::TryCatchStmtContext>(_localctx);
      enterOuterAlt(_localctx, 16);
      setState(143);
      match(TzdLangParser::KW_TRY);
      setState(144);
      block();
      setState(145);
      match(TzdLangParser::KW_CATCH);
      setState(146);
      match(TzdLangParser::T__2);
      setState(147);
      match(TzdLangParser::IDENTIFIER);
      setState(148);
      match(TzdLangParser::T__3);
      setState(149);
      block();
      break;
    }

    case 17: {
      _localctx = _tracker.createInstance<TzdLangParser::ThrowStmtContext>(_localctx);
      enterOuterAlt(_localctx, 17);
      setState(151);
      match(TzdLangParser::KW_THROW);
      setState(152);
      expression(0);
      setState(153);
      match(TzdLangParser::T__1);
      break;
    }

    case 18: {
      _localctx = _tracker.createInstance<TzdLangParser::ExprStmtContext>(_localctx);
      enterOuterAlt(_localctx, 18);
      setState(155);
      expression(0);
      setState(156);
      match(TzdLangParser::T__1);
      break;
    }

    case 19: {
      _localctx = _tracker.createInstance<TzdLangParser::EmptyStmtContext>(_localctx);
      enterOuterAlt(_localctx, 19);
      setState(158);
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
    setState(161);
    match(TzdLangParser::KW_CASE);
    setState(162);
    expression(0);
    setState(163);
    match(TzdLangParser::T__6);
    setState(167);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 6916389856809722156) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 66)) & 245761) != 0)) {
      setState(164);
      statement();
      setState(169);
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
    setState(170);
    match(TzdLangParser::KW_DEFAULT);
    setState(171);
    match(TzdLangParser::T__6);
    setState(175);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 6916389856809722156) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 66)) & 245761) != 0)) {
      setState(172);
      statement();
      setState(177);
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
    setState(178);
    match(TzdLangParser::KW_CLASS);
    setState(179);
    match(TzdLangParser::T__7);
    setState(180);
    match(TzdLangParser::IDENTIFIER);
    setState(181);
    match(TzdLangParser::T__2);
    setState(183);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 34) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
      setState(182);
      paramList();
    }
    setState(185);
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
    setState(187);
    match(TzdLangParser::KW_ENUM);
    setState(188);
    match(TzdLangParser::IDENTIFIER);
    setState(189);
    match(TzdLangParser::T__4);
    setState(191);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::IDENTIFIER) {
      setState(190);
      enumList();
    }
    setState(193);
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
    setState(195);
    match(TzdLangParser::IDENTIFIER);
    setState(200);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(196);
      match(TzdLangParser::T__8);
      setState(197);
      match(TzdLangParser::IDENTIFIER);
      setState(202);
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
    setState(229);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(204);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(203);
        annotationUsage();
      }
      setState(206);
      match(TzdLangParser::KW_CLASS);
      setState(207);
      qualifiedName();
      setState(210);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__6) {
        setState(208);
        match(TzdLangParser::T__6);
        setState(209);
        qualifiedName();
      }
      setState(212);
      match(TzdLangParser::T__4);
      setState(213);
      classBody();
      setState(214);
      match(TzdLangParser::T__5);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(217);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(216);
        annotationUsage();
      }
      setState(219);
      match(TzdLangParser::KW_CLASS);
      setState(220);
      qualifiedName();
      setState(223);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::KW_EXTENDS) {
        setState(221);
        match(TzdLangParser::KW_EXTENDS);
        setState(222);
        qualifiedName();
      }
      setState(225);
      match(TzdLangParser::T__4);
      setState(226);
      classBody();
      setState(227);
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
    setState(234);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 16143745280) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(231);
      classMember();
      setState(236);
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
    setState(245);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(238);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__7) {
        setState(237);
        annotationUsage();
      }
      setState(241);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 7516192768) != 0)) {
        setState(240);
        accessModifier();
      }
      setState(243);
      memberDecl();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(244);
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
    setState(308);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::KW_VAR: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldVarDeclContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(247);
        match(TzdLangParser::KW_VAR);
        setState(248);
        typeType(0);
        setState(249);
        match(TzdLangParser::IDENTIFIER);
        setState(252);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ASSIGN) {
          setState(250);
          match(TzdLangParser::ASSIGN);
          setState(251);
          expression(0);
        }
        setState(254);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_LET: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldLetDeclContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(256);
        match(TzdLangParser::KW_LET);
        setState(257);
        typeType(0);
        setState(258);
        match(TzdLangParser::IDENTIFIER);
        setState(261);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::ASSIGN) {
          setState(259);
          match(TzdLangParser::ASSIGN);
          setState(260);
          expression(0);
        }
        setState(263);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_CONST: {
        _localctx = _tracker.createInstance<TzdLangParser::FieldConstDeclContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(265);
        match(TzdLangParser::KW_CONST);
        setState(266);
        match(TzdLangParser::IDENTIFIER);
        setState(269);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == TzdLangParser::T__6) {
          setState(267);
          match(TzdLangParser::T__6);
          setState(268);
          typeType(0);
        }
        setState(271);
        match(TzdLangParser::ASSIGN);
        setState(272);
        expression(0);
        setState(273);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_STATIC: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodStaticDeclContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(275);
        match(TzdLangParser::KW_STATIC);
        setState(276);
        match(TzdLangParser::KW_FUN);
        setState(277);
        match(TzdLangParser::IDENTIFIER);
        setState(278);
        match(TzdLangParser::T__2);
        setState(280);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 34) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
          setState(279);
          paramList();
        }
        setState(282);
        match(TzdLangParser::T__3);
        setState(283);
        block();
        break;
      }

      case TzdLangParser::KW_ABSTRACT: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodAbstractDeclContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(284);
        match(TzdLangParser::KW_ABSTRACT);
        setState(285);
        match(TzdLangParser::KW_FUN);
        setState(286);
        match(TzdLangParser::IDENTIFIER);
        setState(287);
        match(TzdLangParser::T__2);
        setState(289);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 34) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
          setState(288);
          paramList();
        }
        setState(291);
        match(TzdLangParser::T__3);
        setState(292);
        match(TzdLangParser::T__1);
        break;
      }

      case TzdLangParser::KW_FUN: {
        _localctx = _tracker.createInstance<TzdLangParser::MethodDeclContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(293);
        match(TzdLangParser::KW_FUN);
        setState(294);
        match(TzdLangParser::IDENTIFIER);
        setState(295);
        match(TzdLangParser::T__2);
        setState(297);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 34) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
          setState(296);
          paramList();
        }
        setState(299);
        match(TzdLangParser::T__3);
        setState(300);
        block();
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        _localctx = _tracker.createInstance<TzdLangParser::ConstructorDeclContext>(_localctx);
        enterOuterAlt(_localctx, 7);
        setState(301);
        match(TzdLangParser::IDENTIFIER);
        setState(302);
        match(TzdLangParser::T__2);
        setState(304);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 34) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
          setState(303);
          paramList();
        }
        setState(306);
        match(TzdLangParser::T__3);
        setState(307);
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
    setState(310);
    match(TzdLangParser::T__7);
    setState(311);
    match(TzdLangParser::IDENTIFIER);
    setState(317);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__2) {
      setState(312);
      match(TzdLangParser::T__2);
      setState(314);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 66)) & 245761) != 0)) {
        setState(313);
        exprList();
      }
      setState(316);
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
    setState(319);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 7516192768) != 0))) {
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
    setState(322);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__7) {
      setState(321);
      annotationUsage();
    }
    setState(324);
    match(TzdLangParser::KW_FUN);
    setState(325);
    match(TzdLangParser::IDENTIFIER);
    setState(326);
    match(TzdLangParser::T__2);
    setState(328);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 34) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
      setState(327);
      paramList();
    }
    setState(330);
    match(TzdLangParser::T__3);
    setState(331);
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
    setState(334);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == TzdLangParser::T__7) {
      setState(333);
      annotationUsage();
    }
    setState(336);
    match(TzdLangParser::KW_NATIVE);
    setState(337);
    match(TzdLangParser::KW_FUN);
    setState(338);
    match(TzdLangParser::IDENTIFIER);
    setState(339);
    match(TzdLangParser::T__2);
    setState(341);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 34) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
      setState(340);
      paramList();
    }
    setState(343);
    match(TzdLangParser::T__3);
    setState(344);
    match(TzdLangParser::T__2);
    setState(346);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 285978602107837440) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(345);
      nativeAttrList();
    }
    setState(348);
    match(TzdLangParser::T__3);
    setState(349);
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
    setState(351);
    nativeAttr();
    setState(356);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(352);
      match(TzdLangParser::T__8);
      setState(353);
      nativeAttr();
      setState(358);
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
    setState(361);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::IDENTIFIER: {
        setState(359);
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
        setState(360);
        nativePropKey();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(363);
    match(TzdLangParser::ASSIGN);
    setState(364);
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
    setState(366);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 285978602107837440) != 0))) {
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
    setState(387);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 44, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(368);
      typeType(0);
      setState(369);
      match(TzdLangParser::IDENTIFIER);
      setState(372);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(370);
        match(TzdLangParser::ASSIGN);
        setState(371);
        expression(0);
      }
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(374);
      match(TzdLangParser::KW_VAR);
      setState(375);
      match(TzdLangParser::IDENTIFIER);
      setState(378);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(376);
        match(TzdLangParser::ASSIGN);
        setState(377);
        expression(0);
      }
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(380);
      match(TzdLangParser::IDENTIFIER);
      setState(381);
      match(TzdLangParser::T__6);
      setState(382);
      typeType(0);
      setState(385);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::ASSIGN) {
        setState(383);
        match(TzdLangParser::ASSIGN);
        setState(384);
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
    setState(391);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 45, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(389);
      variableDeclaration();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(390);
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
    setState(393);
    match(TzdLangParser::KW_IMPORT);
    setState(394);
    match(TzdLangParser::STRING);
    setState(395);
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
    setState(397);
    param();
    setState(402);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(398);
      match(TzdLangParser::T__8);
      setState(399);
      param();
      setState(404);
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
    setState(413);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 48, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(405);
      typeType(0);
      setState(406);
      match(TzdLangParser::IDENTIFIER);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(408);
      _la = _input->LA(1);
      if (!(((((_la - 34) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 34)) & 70368760823809) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(411);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == TzdLangParser::T__6) {
        setState(409);
        match(TzdLangParser::T__6);
        setState(410);
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
    setState(415);
    match(TzdLangParser::T__4);
    setState(419);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 6916389856809722156) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 66)) & 245761) != 0)) {
      setState(416);
      statement();
      setState(421);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(422);
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
//----------------- UnaryExprContext ------------------------------------------------------------------

TzdLangParser::ExpressionContext* TzdLangParser::UnaryExprContext::expression() {
  return getRuleContext<TzdLangParser::ExpressionContext>(0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::MINUS() {
  return getToken(TzdLangParser::MINUS, 0);
}

tree::TerminalNode* TzdLangParser::UnaryExprContext::NOT() {
  return getToken(TzdLangParser::NOT, 0);
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
    setState(435);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 50, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<CastExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(425);
      match(TzdLangParser::T__2);
      setState(426);
      typeType(0);
      setState(427);
      match(TzdLangParser::T__3);
      setState(428);
      expression(15);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<PrefixExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(430);
      _la = _input->LA(1);
      if (!(_la == TzdLangParser::INC

      || _la == TzdLangParser::DEC)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(431);
      expression(11);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<UnaryExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(432);
      _la = _input->LA(1);
      if (!(((((_la - 60) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 60)) & 69) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(433);
      expression(10);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<AtomExprContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(434);
      atom(0);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(476);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 53, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(474);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 52, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<PowerExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(437);

          if (!(precpred(_ctx, 12))) throw FailedPredicateException(this, "precpred(_ctx, 12)");
          setState(438);
          match(TzdLangParser::T__14);
          setState(439);
          expression(13);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<MultiplicativeExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(440);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(441);
          _la = _input->LA(1);
          if (!(((((_la - 63) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 63)) & 7) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(442);
          expression(9);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<AdditiveExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(443);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(444);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::PLUS

          || _la == TzdLangParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(445);
          expression(8);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<RelationalExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(446);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(447);
          _la = _input->LA(1);
          if (!(((((_la - 67) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 67)) & 15) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(448);
          expression(7);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<EqualityExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(449);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(450);
          _la = _input->LA(1);
          if (!(_la == TzdLangParser::EEQ

          || _la == TzdLangParser::NEQ)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(451);
          expression(6);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<LogicalAndExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(452);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(453);
          match(TzdLangParser::AND);
          setState(454);
          expression(5);
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<LogicalOrExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(455);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(456);
          match(TzdLangParser::OR);
          setState(457);
          expression(4);
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<AssignmentExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(458);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(459);
          _la = _input->LA(1);
          if (!(((((_la - 75) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 75)) & 31) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(460);
          expression(2);
          break;
        }

        case 9: {
          auto newContext = _tracker.createInstance<IndexExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(461);

          if (!(precpred(_ctx, 14))) throw FailedPredicateException(this, "precpred(_ctx, 14)");
          setState(462);
          match(TzdLangParser::T__12);
          setState(463);
          expression(0);
          setState(464);
          match(TzdLangParser::T__13);
          break;
        }

        case 10: {
          auto newContext = _tracker.createInstance<PostfixExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(466);

          if (!(precpred(_ctx, 13))) throw FailedPredicateException(this, "precpred(_ctx, 13)");
          setState(467);
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

        case 11: {
          auto newContext = _tracker.createInstance<TypeCheckExprContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(468);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(469);
          match(TzdLangParser::KW_IN);
          setState(472);
          _errHandler->sync(this);
          switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 51, _ctx)) {
          case 1: {
            setState(470);
            qualifiedName();
            break;
          }

          case 2: {
            setState(471);
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
      setState(478);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 53, _ctx);
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
    setState(525);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::T__2: {
        _localctx = _tracker.createInstance<ParenExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(480);
        match(TzdLangParser::T__2);
        setState(481);
        expression(0);
        setState(482);
        match(TzdLangParser::T__3);
        break;
      }

      case TzdLangParser::T__12: {
        _localctx = _tracker.createInstance<ArrayLiteralExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(484);
        match(TzdLangParser::T__12);
        setState(486);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 66)) & 245761) != 0)) {
          setState(485);
          exprList();
        }
        setState(488);
        match(TzdLangParser::T__13);
        break;
      }

      case TzdLangParser::T__4: {
        _localctx = _tracker.createInstance<ArrayLiteralExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(489);
        match(TzdLangParser::T__4);
        setState(491);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 66)) & 245761) != 0)) {
          setState(490);
          exprList();
        }
        setState(493);
        match(TzdLangParser::T__5);
        break;
      }

      case TzdLangParser::INTEGER: {
        _localctx = _tracker.createInstance<IntExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(494);
        match(TzdLangParser::INTEGER);
        break;
      }

      case TzdLangParser::FLOAT: {
        _localctx = _tracker.createInstance<FloatExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(495);
        match(TzdLangParser::FLOAT);
        break;
      }

      case TzdLangParser::STRING: {
        _localctx = _tracker.createInstance<StringExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(496);
        match(TzdLangParser::STRING);
        break;
      }

      case TzdLangParser::KW_TRUE: {
        _localctx = _tracker.createInstance<BoolTrueExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(497);
        match(TzdLangParser::KW_TRUE);
        break;
      }

      case TzdLangParser::KW_FALSE: {
        _localctx = _tracker.createInstance<BoolFalseExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(498);
        match(TzdLangParser::KW_FALSE);
        break;
      }

      case TzdLangParser::KW_NULL: {
        _localctx = _tracker.createInstance<NullExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(499);
        match(TzdLangParser::KW_NULL);
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        _localctx = _tracker.createInstance<IdExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(500);
        match(TzdLangParser::IDENTIFIER);
        break;
      }

      case TzdLangParser::KW_SUPER: {
        _localctx = _tracker.createInstance<SuperExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(501);
        match(TzdLangParser::KW_SUPER);
        setState(502);
        match(TzdLangParser::T__2);
        setState(504);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 66)) & 245761) != 0)) {
          setState(503);
          exprList();
        }
        setState(506);
        match(TzdLangParser::T__3);
        break;
      }

      case TzdLangParser::KW_PRINT: {
        _localctx = _tracker.createInstance<PrintFunExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(507);
        printFunction();
        break;
      }

      case TzdLangParser::KW_NEW: {
        _localctx = _tracker.createInstance<NewExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(508);
        match(TzdLangParser::KW_NEW);
        setState(509);
        qualifiedName();
        setState(510);
        match(TzdLangParser::T__2);
        setState(512);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 66)) & 245761) != 0)) {
          setState(511);
          exprList();
        }
        setState(514);
        match(TzdLangParser::T__3);
        setState(516);
        _errHandler->sync(this);

        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 58, _ctx)) {
        case 1: {
          setState(515);
          classOverrideBlock();
          break;
        }

        default:
          break;
        }
        break;
      }

      case TzdLangParser::KW_FUN: {
        _localctx = _tracker.createInstance<LambdaExprContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(518);
        match(TzdLangParser::KW_FUN);
        setState(519);
        match(TzdLangParser::T__2);
        setState(521);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (((((_la - 34) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 34)) & 70368760823809) != 0)) {
          setState(520);
          paramList();
        }
        setState(523);
        match(TzdLangParser::T__3);
        setState(524);
        block();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(538);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 63, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(536);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 62, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<CallExprContext>(_tracker.createInstance<AtomContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleAtom);
          setState(527);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(528);
          match(TzdLangParser::T__2);
          setState(530);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if ((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
            ((1ULL << (_la - 66)) & 245761) != 0)) {
            setState(529);
            exprList();
          }
          setState(532);
          match(TzdLangParser::T__3);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<MemberAccessExprContext>(_tracker.createInstance<AtomContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleAtom);
          setState(533);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(534);
          match(TzdLangParser::T__0);
          setState(535);
          match(TzdLangParser::IDENTIFIER);
          break;
        }

        default:
          break;
        } 
      }
      setState(540);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 63, _ctx);
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
    setState(541);
    match(TzdLangParser::T__4);
    setState(545);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 16143745280) != 0) || _la == TzdLangParser::IDENTIFIER) {
      setState(542);
      classMember();
      setState(547);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(548);
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
    setState(550);
    match(TzdLangParser::KW_PRINT);
    setState(551);
    match(TzdLangParser::T__2);
    setState(553);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 6629562543020974120) != 0) || ((((_la - 66) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 66)) & 245761) != 0)) {
      setState(552);
      exprList();
    }
    setState(555);
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
    setState(557);
    expression(0);
    setState(562);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == TzdLangParser::T__8) {
      setState(558);
      match(TzdLangParser::T__8);
      setState(559);
      expression(0);
      setState(564);
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
  size_t startState = 60;
  enterRecursionRule(_localctx, 60, TzdLangParser::RuleTypeType, precedence);

    

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
    setState(574);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case TzdLangParser::T_INT: {
        setState(566);
        match(TzdLangParser::T_INT);
        break;
      }

      case TzdLangParser::T_FLOAT: {
        setState(567);
        match(TzdLangParser::T_FLOAT);
        break;
      }

      case TzdLangParser::T_STRING: {
        setState(568);
        match(TzdLangParser::T_STRING);
        break;
      }

      case TzdLangParser::T_BOOL: {
        setState(569);
        match(TzdLangParser::T_BOOL);
        break;
      }

      case TzdLangParser::T_VOID: {
        setState(570);
        match(TzdLangParser::T_VOID);
        break;
      }

      case TzdLangParser::T_PTR: {
        setState(571);
        match(TzdLangParser::T_PTR);
        break;
      }

      case TzdLangParser::T_FUNCTION: {
        setState(572);
        match(TzdLangParser::T_FUNCTION);
        break;
      }

      case TzdLangParser::IDENTIFIER: {
        setState(573);
        qualifiedName();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(580);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 68, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        _localctx = _tracker.createInstance<TypeTypeContext>(parentContext, parentState);
        pushNewRecursionContext(_localctx, startState, RuleTypeType);
        setState(576);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(577);
        match(TzdLangParser::T__15); 
      }
      setState(582);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 68, _ctx);
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
    case 30: return typeTypeSempred(antlrcpp::downCast<TypeTypeContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool TzdLangParser::expressionSempred(ExpressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 12);
    case 1: return precpred(_ctx, 8);
    case 2: return precpred(_ctx, 7);
    case 3: return precpred(_ctx, 6);
    case 4: return precpred(_ctx, 5);
    case 5: return precpred(_ctx, 4);
    case 6: return precpred(_ctx, 3);
    case 7: return precpred(_ctx, 2);
    case 8: return precpred(_ctx, 14);
    case 9: return precpred(_ctx, 13);
    case 10: return precpred(_ctx, 9);

  default:
    break;
  }
  return true;
}

bool TzdLangParser::atomSempred(AtomContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 11: return precpred(_ctx, 4);
    case 12: return precpred(_ctx, 3);

  default:
    break;
  }
  return true;
}

bool TzdLangParser::typeTypeSempred(TypeTypeContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 13: return precpred(_ctx, 1);

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
