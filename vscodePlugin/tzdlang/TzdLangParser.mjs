// Generated from Grammar/TzdLang.g4 by ANTLR 4.13.1
// jshint ignore: start
import antlr4 from 'antlr4';
import TzdLangListener from './TzdLangListener.mjs';
import TzdLangVisitor from './TzdLangVisitor.mjs';

const serializedATN = [4,1,101,655,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,
4,2,5,7,5,2,6,7,6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,
2,13,7,13,2,14,7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,
20,7,20,2,21,7,21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,
7,27,2,28,7,28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,1,0,5,0,
70,8,0,10,0,12,0,73,9,0,1,0,1,0,1,1,1,1,1,1,5,1,80,8,1,10,1,12,1,83,9,1,
1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,97,8,2,1,2,1,2,1,2,1,
2,1,2,1,2,1,2,1,2,3,2,107,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,118,
8,2,1,2,1,2,3,2,122,8,2,1,2,1,2,3,2,126,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,
1,2,1,2,1,2,1,2,1,2,5,2,140,8,2,10,2,12,2,143,9,2,1,2,3,2,146,8,2,1,2,1,
2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,166,
8,2,1,3,1,3,1,3,1,3,5,3,172,8,3,10,3,12,3,175,9,3,1,4,1,4,1,4,5,4,180,8,
4,10,4,12,4,183,9,4,1,5,1,5,1,5,1,5,1,5,3,5,190,8,5,1,5,1,5,1,6,1,6,1,6,
1,6,3,6,198,8,6,1,6,1,6,1,7,1,7,1,7,5,7,205,8,7,10,7,12,7,208,9,7,1,8,3,
8,211,8,8,1,8,1,8,1,8,1,8,3,8,217,8,8,1,8,1,8,1,8,1,8,1,8,3,8,224,8,8,1,
8,1,8,1,8,1,8,3,8,230,8,8,1,8,1,8,1,8,1,8,3,8,236,8,8,1,9,5,9,239,8,9,10,
9,12,9,242,9,9,1,10,3,10,245,8,10,1,10,3,10,248,8,10,1,10,1,10,3,10,252,
8,10,1,11,1,11,1,11,1,11,1,11,3,11,259,8,11,1,11,1,11,1,11,1,11,1,11,1,11,
1,11,3,11,268,8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,276,8,11,1,11,1,11,
1,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,287,8,11,1,11,1,11,1,11,3,11,292,
8,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,300,8,11,1,11,1,11,1,11,3,11,305,
8,11,1,11,1,11,1,11,1,11,1,11,3,11,312,8,11,1,11,1,11,1,11,3,11,317,8,11,
1,11,1,11,1,11,1,11,3,11,323,8,11,1,11,1,11,3,11,327,8,11,1,12,1,12,1,12,
1,12,3,12,333,8,12,1,12,3,12,336,8,12,1,13,1,13,1,14,3,14,341,8,14,1,14,
1,14,1,14,1,14,3,14,347,8,14,1,14,1,14,1,14,3,14,352,8,14,1,14,1,14,1,15,
3,15,357,8,15,1,15,1,15,1,15,1,15,1,15,3,15,364,8,15,1,15,1,15,1,15,3,15,
369,8,15,1,15,1,15,3,15,373,8,15,1,15,1,15,1,15,1,16,1,16,1,16,5,16,381,
8,16,10,16,12,16,384,9,16,1,17,1,17,3,17,388,8,17,1,17,1,17,1,17,1,18,1,
18,1,19,1,19,1,19,1,19,3,19,399,8,19,1,19,1,19,1,19,1,19,3,19,405,8,19,1,
19,1,19,1,19,1,19,1,19,3,19,412,8,19,3,19,414,8,19,1,20,1,20,3,20,418,8,
20,1,21,1,21,1,21,1,21,1,22,1,22,1,22,5,22,427,8,22,10,22,12,22,430,9,22,
1,23,1,23,1,23,1,23,1,23,1,23,3,23,438,8,23,3,23,440,8,23,1,24,1,24,5,24,
444,8,24,10,24,12,24,447,9,24,1,24,1,24,1,25,1,25,1,25,1,25,1,25,1,25,1,
25,1,25,1,25,1,25,1,25,3,25,462,8,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,3,25,511,8,25,5,25,
513,8,25,10,25,12,25,516,9,25,1,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,525,
8,26,1,26,1,26,1,26,3,26,530,8,26,1,26,1,26,1,26,3,26,535,8,26,1,26,1,26,
1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,3,26,548,8,26,1,26,1,26,1,26,
1,26,1,26,1,26,3,26,556,8,26,1,26,1,26,3,26,560,8,26,1,26,1,26,1,26,3,26,
565,8,26,1,26,1,26,1,26,3,26,570,8,26,1,26,3,26,573,8,26,1,26,1,26,1,26,
3,26,578,8,26,1,26,1,26,1,26,1,26,5,26,584,8,26,10,26,12,26,587,9,26,1,27,
1,27,5,27,591,8,27,10,27,12,27,594,9,27,1,27,1,27,1,28,1,28,1,28,3,28,601,
8,28,1,28,1,28,1,29,1,29,1,29,5,29,608,8,29,10,29,12,29,611,9,29,1,30,1,
30,1,30,5,30,616,8,30,10,30,12,30,619,9,30,1,30,3,30,622,8,30,1,31,1,31,
1,31,1,31,1,32,1,32,1,32,1,32,1,32,1,32,1,32,3,32,635,8,32,1,33,1,33,1,33,
1,33,1,33,1,33,1,33,1,33,1,33,3,33,646,8,33,1,33,1,33,5,33,650,8,33,10,33,
12,33,653,9,33,1,33,0,3,50,52,66,34,0,2,4,6,8,10,12,14,16,18,20,22,24,26,
28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,0,12,1,0,29,
31,2,0,96,96,98,98,3,0,10,12,32,33,50,56,3,0,33,33,50,56,95,95,1,0,57,58,
4,0,59,59,81,81,86,87,91,91,1,0,88,90,1,0,86,87,1,0,63,65,2,0,76,77,92,93,
1,0,74,75,3,0,60,62,66,73,94,94,758,0,71,1,0,0,0,2,76,1,0,0,0,4,165,1,0,
0,0,6,167,1,0,0,0,8,176,1,0,0,0,10,184,1,0,0,0,12,193,1,0,0,0,14,201,1,0,
0,0,16,235,1,0,0,0,18,240,1,0,0,0,20,251,1,0,0,0,22,326,1,0,0,0,24,328,1,
0,0,0,26,337,1,0,0,0,28,340,1,0,0,0,30,356,1,0,0,0,32,377,1,0,0,0,34,387,
1,0,0,0,36,392,1,0,0,0,38,413,1,0,0,0,40,417,1,0,0,0,42,419,1,0,0,0,44,423,
1,0,0,0,46,439,1,0,0,0,48,441,1,0,0,0,50,461,1,0,0,0,52,572,1,0,0,0,54,588,
1,0,0,0,56,597,1,0,0,0,58,604,1,0,0,0,60,612,1,0,0,0,62,623,1,0,0,0,64,634,
1,0,0,0,66,645,1,0,0,0,68,70,3,4,2,0,69,68,1,0,0,0,70,73,1,0,0,0,71,69,1,
0,0,0,71,72,1,0,0,0,72,74,1,0,0,0,73,71,1,0,0,0,74,75,5,0,0,1,75,1,1,0,0,
0,76,81,5,95,0,0,77,78,5,1,0,0,78,80,5,95,0,0,79,77,1,0,0,0,80,83,1,0,0,
0,81,79,1,0,0,0,81,82,1,0,0,0,82,3,1,0,0,0,83,81,1,0,0,0,84,166,3,48,24,
0,85,166,3,16,8,0,86,166,3,10,5,0,87,166,3,12,6,0,88,166,3,28,14,0,89,166,
3,30,15,0,90,91,3,38,19,0,91,92,5,2,0,0,92,166,1,0,0,0,93,166,3,42,21,0,
94,96,5,33,0,0,95,97,3,50,25,0,96,95,1,0,0,0,96,97,1,0,0,0,97,98,1,0,0,0,
98,166,5,2,0,0,99,100,5,34,0,0,100,101,5,3,0,0,101,102,3,50,25,0,102,103,
5,4,0,0,103,106,3,4,2,0,104,105,5,35,0,0,105,107,3,4,2,0,106,104,1,0,0,0,
106,107,1,0,0,0,107,166,1,0,0,0,108,109,5,36,0,0,109,110,5,3,0,0,110,111,
3,50,25,0,111,112,5,4,0,0,112,113,3,4,2,0,113,166,1,0,0,0,114,115,5,37,0,
0,115,117,5,3,0,0,116,118,3,40,20,0,117,116,1,0,0,0,117,118,1,0,0,0,118,
119,1,0,0,0,119,121,5,2,0,0,120,122,3,50,25,0,121,120,1,0,0,0,121,122,1,
0,0,0,122,123,1,0,0,0,123,125,5,2,0,0,124,126,3,50,25,0,125,124,1,0,0,0,
125,126,1,0,0,0,126,127,1,0,0,0,127,128,5,4,0,0,128,166,3,4,2,0,129,130,
5,38,0,0,130,166,5,2,0,0,131,132,5,39,0,0,132,166,5,2,0,0,133,134,5,40,0,
0,134,135,5,3,0,0,135,136,3,50,25,0,136,137,5,4,0,0,137,141,5,5,0,0,138,
140,3,6,3,0,139,138,1,0,0,0,140,143,1,0,0,0,141,139,1,0,0,0,141,142,1,0,
0,0,142,145,1,0,0,0,143,141,1,0,0,0,144,146,3,8,4,0,145,144,1,0,0,0,145,
146,1,0,0,0,146,147,1,0,0,0,147,148,5,6,0,0,148,166,1,0,0,0,149,150,5,48,
0,0,150,151,3,48,24,0,151,152,5,49,0,0,152,153,5,3,0,0,153,154,5,95,0,0,
154,155,5,4,0,0,155,156,3,48,24,0,156,166,1,0,0,0,157,158,5,47,0,0,158,159,
3,50,25,0,159,160,5,2,0,0,160,166,1,0,0,0,161,162,3,50,25,0,162,163,5,2,
0,0,163,166,1,0,0,0,164,166,5,2,0,0,165,84,1,0,0,0,165,85,1,0,0,0,165,86,
1,0,0,0,165,87,1,0,0,0,165,88,1,0,0,0,165,89,1,0,0,0,165,90,1,0,0,0,165,
93,1,0,0,0,165,94,1,0,0,0,165,99,1,0,0,0,165,108,1,0,0,0,165,114,1,0,0,0,
165,129,1,0,0,0,165,131,1,0,0,0,165,133,1,0,0,0,165,149,1,0,0,0,165,157,
1,0,0,0,165,161,1,0,0,0,165,164,1,0,0,0,166,5,1,0,0,0,167,168,5,41,0,0,168,
169,3,50,25,0,169,173,5,7,0,0,170,172,3,4,2,0,171,170,1,0,0,0,172,175,1,
0,0,0,173,171,1,0,0,0,173,174,1,0,0,0,174,7,1,0,0,0,175,173,1,0,0,0,176,
177,5,42,0,0,177,181,5,7,0,0,178,180,3,4,2,0,179,178,1,0,0,0,180,183,1,0,
0,0,181,179,1,0,0,0,181,182,1,0,0,0,182,9,1,0,0,0,183,181,1,0,0,0,184,185,
5,27,0,0,185,186,5,8,0,0,186,187,5,95,0,0,187,189,5,3,0,0,188,190,3,44,22,
0,189,188,1,0,0,0,189,190,1,0,0,0,190,191,1,0,0,0,191,192,5,4,0,0,192,11,
1,0,0,0,193,194,5,21,0,0,194,195,5,95,0,0,195,197,5,5,0,0,196,198,3,14,7,
0,197,196,1,0,0,0,197,198,1,0,0,0,198,199,1,0,0,0,199,200,5,6,0,0,200,13,
1,0,0,0,201,206,5,95,0,0,202,203,5,9,0,0,203,205,5,95,0,0,204,202,1,0,0,
0,205,208,1,0,0,0,206,204,1,0,0,0,206,207,1,0,0,0,207,15,1,0,0,0,208,206,
1,0,0,0,209,211,3,24,12,0,210,209,1,0,0,0,210,211,1,0,0,0,211,212,1,0,0,
0,212,213,5,27,0,0,213,216,3,2,1,0,214,215,5,7,0,0,215,217,3,2,1,0,216,214,
1,0,0,0,216,217,1,0,0,0,217,218,1,0,0,0,218,219,5,5,0,0,219,220,3,18,9,0,
220,221,5,6,0,0,221,236,1,0,0,0,222,224,3,24,12,0,223,222,1,0,0,0,223,224,
1,0,0,0,224,225,1,0,0,0,225,226,5,27,0,0,226,229,3,2,1,0,227,228,5,28,0,
0,228,230,3,2,1,0,229,227,1,0,0,0,229,230,1,0,0,0,230,231,1,0,0,0,231,232,
5,5,0,0,232,233,3,18,9,0,233,234,5,6,0,0,234,236,1,0,0,0,235,210,1,0,0,0,
235,223,1,0,0,0,236,17,1,0,0,0,237,239,3,20,10,0,238,237,1,0,0,0,239,242,
1,0,0,0,240,238,1,0,0,0,240,241,1,0,0,0,241,19,1,0,0,0,242,240,1,0,0,0,243,
245,3,24,12,0,244,243,1,0,0,0,244,245,1,0,0,0,245,247,1,0,0,0,246,248,3,
26,13,0,247,246,1,0,0,0,247,248,1,0,0,0,248,249,1,0,0,0,249,252,3,22,11,
0,250,252,3,30,15,0,251,244,1,0,0,0,251,250,1,0,0,0,252,21,1,0,0,0,253,254,
5,16,0,0,254,255,3,66,33,0,255,258,5,95,0,0,256,257,5,94,0,0,257,259,3,50,
25,0,258,256,1,0,0,0,258,259,1,0,0,0,259,260,1,0,0,0,260,261,5,2,0,0,261,
327,1,0,0,0,262,263,5,18,0,0,263,264,3,66,33,0,264,267,5,95,0,0,265,266,
5,94,0,0,266,268,3,50,25,0,267,265,1,0,0,0,267,268,1,0,0,0,268,269,1,0,0,
0,269,270,5,2,0,0,270,327,1,0,0,0,271,272,5,17,0,0,272,275,5,95,0,0,273,
274,5,7,0,0,274,276,3,66,33,0,275,273,1,0,0,0,275,276,1,0,0,0,276,277,1,
0,0,0,277,278,5,94,0,0,278,279,3,50,25,0,279,280,5,2,0,0,280,327,1,0,0,0,
281,282,5,19,0,0,282,283,5,32,0,0,283,284,5,95,0,0,284,286,5,3,0,0,285,287,
3,44,22,0,286,285,1,0,0,0,286,287,1,0,0,0,287,288,1,0,0,0,288,291,5,4,0,
0,289,290,5,85,0,0,290,292,3,66,33,0,291,289,1,0,0,0,291,292,1,0,0,0,292,
293,1,0,0,0,293,327,3,48,24,0,294,295,5,20,0,0,295,296,5,32,0,0,296,297,
5,95,0,0,297,299,5,3,0,0,298,300,3,44,22,0,299,298,1,0,0,0,299,300,1,0,0,
0,300,301,1,0,0,0,301,304,5,4,0,0,302,303,5,85,0,0,303,305,3,66,33,0,304,
302,1,0,0,0,304,305,1,0,0,0,305,306,1,0,0,0,306,327,5,2,0,0,307,308,5,32,
0,0,308,309,5,95,0,0,309,311,5,3,0,0,310,312,3,44,22,0,311,310,1,0,0,0,311,
312,1,0,0,0,312,313,1,0,0,0,313,316,5,4,0,0,314,315,5,85,0,0,315,317,3,66,
33,0,316,314,1,0,0,0,316,317,1,0,0,0,317,318,1,0,0,0,318,327,3,48,24,0,319,
320,5,95,0,0,320,322,5,3,0,0,321,323,3,44,22,0,322,321,1,0,0,0,322,323,1,
0,0,0,323,324,1,0,0,0,324,325,5,4,0,0,325,327,3,48,24,0,326,253,1,0,0,0,
326,262,1,0,0,0,326,271,1,0,0,0,326,281,1,0,0,0,326,294,1,0,0,0,326,307,
1,0,0,0,326,319,1,0,0,0,327,23,1,0,0,0,328,329,5,8,0,0,329,335,5,95,0,0,
330,332,5,3,0,0,331,333,3,58,29,0,332,331,1,0,0,0,332,333,1,0,0,0,333,334,
1,0,0,0,334,336,5,4,0,0,335,330,1,0,0,0,335,336,1,0,0,0,336,25,1,0,0,0,337,
338,7,0,0,0,338,27,1,0,0,0,339,341,3,24,12,0,340,339,1,0,0,0,340,341,1,0,
0,0,341,342,1,0,0,0,342,343,5,32,0,0,343,344,5,95,0,0,344,346,5,3,0,0,345,
347,3,44,22,0,346,345,1,0,0,0,346,347,1,0,0,0,347,348,1,0,0,0,348,351,5,
4,0,0,349,350,5,85,0,0,350,352,3,66,33,0,351,349,1,0,0,0,351,352,1,0,0,0,
352,353,1,0,0,0,353,354,3,48,24,0,354,29,1,0,0,0,355,357,3,24,12,0,356,355,
1,0,0,0,356,357,1,0,0,0,357,358,1,0,0,0,358,359,5,24,0,0,359,360,5,32,0,
0,360,361,5,95,0,0,361,363,5,3,0,0,362,364,3,44,22,0,363,362,1,0,0,0,363,
364,1,0,0,0,364,365,1,0,0,0,365,368,5,4,0,0,366,367,5,85,0,0,367,369,3,66,
33,0,368,366,1,0,0,0,368,369,1,0,0,0,369,370,1,0,0,0,370,372,5,3,0,0,371,
373,3,32,16,0,372,371,1,0,0,0,372,373,1,0,0,0,373,374,1,0,0,0,374,375,5,
4,0,0,375,376,5,2,0,0,376,31,1,0,0,0,377,382,3,34,17,0,378,379,5,9,0,0,379,
381,3,34,17,0,380,378,1,0,0,0,381,384,1,0,0,0,382,380,1,0,0,0,382,383,1,
0,0,0,383,33,1,0,0,0,384,382,1,0,0,0,385,388,5,95,0,0,386,388,3,36,18,0,
387,385,1,0,0,0,387,386,1,0,0,0,388,389,1,0,0,0,389,390,5,94,0,0,390,391,
7,1,0,0,391,35,1,0,0,0,392,393,7,2,0,0,393,37,1,0,0,0,394,395,3,66,33,0,
395,398,5,95,0,0,396,397,5,94,0,0,397,399,3,50,25,0,398,396,1,0,0,0,398,
399,1,0,0,0,399,414,1,0,0,0,400,401,5,16,0,0,401,404,5,95,0,0,402,403,5,
94,0,0,403,405,3,50,25,0,404,402,1,0,0,0,404,405,1,0,0,0,405,414,1,0,0,0,
406,407,5,95,0,0,407,408,5,7,0,0,408,411,3,66,33,0,409,410,5,94,0,0,410,
412,3,50,25,0,411,409,1,0,0,0,411,412,1,0,0,0,412,414,1,0,0,0,413,394,1,
0,0,0,413,400,1,0,0,0,413,406,1,0,0,0,414,39,1,0,0,0,415,418,3,38,19,0,416,
418,3,50,25,0,417,415,1,0,0,0,417,416,1,0,0,0,418,41,1,0,0,0,419,420,5,25,
0,0,420,421,5,98,0,0,421,422,5,2,0,0,422,43,1,0,0,0,423,428,3,46,23,0,424,
425,5,9,0,0,425,427,3,46,23,0,426,424,1,0,0,0,427,430,1,0,0,0,428,426,1,
0,0,0,428,429,1,0,0,0,429,45,1,0,0,0,430,428,1,0,0,0,431,432,3,66,33,0,432,
433,5,95,0,0,433,440,1,0,0,0,434,437,7,3,0,0,435,436,5,7,0,0,436,438,3,66,
33,0,437,435,1,0,0,0,437,438,1,0,0,0,438,440,1,0,0,0,439,431,1,0,0,0,439,
434,1,0,0,0,440,47,1,0,0,0,441,445,5,5,0,0,442,444,3,4,2,0,443,442,1,0,0,
0,444,447,1,0,0,0,445,443,1,0,0,0,445,446,1,0,0,0,446,448,1,0,0,0,447,445,
1,0,0,0,448,449,5,6,0,0,449,49,1,0,0,0,450,451,6,25,-1,0,451,452,5,3,0,0,
452,453,3,66,33,0,453,454,5,4,0,0,454,455,3,50,25,19,455,462,1,0,0,0,456,
457,7,4,0,0,457,462,3,50,25,15,458,459,7,5,0,0,459,462,3,50,25,14,460,462,
3,52,26,0,461,450,1,0,0,0,461,456,1,0,0,0,461,458,1,0,0,0,461,460,1,0,0,
0,462,514,1,0,0,0,463,464,10,16,0,0,464,465,5,80,0,0,465,513,3,50,25,16,
466,467,10,12,0,0,467,468,7,6,0,0,468,513,3,50,25,13,469,470,10,11,0,0,470,
471,7,7,0,0,471,513,3,50,25,12,472,473,10,10,0,0,473,474,7,8,0,0,474,513,
3,50,25,11,475,476,10,9,0,0,476,477,7,9,0,0,477,513,3,50,25,10,478,479,10,
8,0,0,479,480,7,10,0,0,480,513,3,50,25,9,481,482,10,7,0,0,482,483,5,82,0,
0,483,513,3,50,25,8,484,485,10,6,0,0,485,486,5,84,0,0,486,513,3,50,25,7,
487,488,10,5,0,0,488,489,5,83,0,0,489,513,3,50,25,6,490,491,10,4,0,0,491,
492,5,78,0,0,492,513,3,50,25,5,493,494,10,3,0,0,494,495,5,79,0,0,495,513,
3,50,25,4,496,497,10,2,0,0,497,498,7,11,0,0,498,513,3,50,25,2,499,500,10,
18,0,0,500,501,5,13,0,0,501,502,3,50,25,0,502,503,5,14,0,0,503,513,1,0,0,
0,504,505,10,17,0,0,505,513,7,4,0,0,506,507,10,13,0,0,507,510,5,22,0,0,508,
511,3,2,1,0,509,511,3,66,33,0,510,508,1,0,0,0,510,509,1,0,0,0,511,513,1,
0,0,0,512,463,1,0,0,0,512,466,1,0,0,0,512,469,1,0,0,0,512,472,1,0,0,0,512,
475,1,0,0,0,512,478,1,0,0,0,512,481,1,0,0,0,512,484,1,0,0,0,512,487,1,0,
0,0,512,490,1,0,0,0,512,493,1,0,0,0,512,496,1,0,0,0,512,499,1,0,0,0,512,
504,1,0,0,0,512,506,1,0,0,0,513,516,1,0,0,0,514,512,1,0,0,0,514,515,1,0,
0,0,515,51,1,0,0,0,516,514,1,0,0,0,517,518,6,26,-1,0,518,519,5,3,0,0,519,
520,3,50,25,0,520,521,5,4,0,0,521,573,1,0,0,0,522,524,5,13,0,0,523,525,3,
58,29,0,524,523,1,0,0,0,524,525,1,0,0,0,525,526,1,0,0,0,526,573,5,14,0,0,
527,529,5,5,0,0,528,530,3,60,30,0,529,528,1,0,0,0,529,530,1,0,0,0,530,531,
1,0,0,0,531,573,5,6,0,0,532,534,5,5,0,0,533,535,3,58,29,0,534,533,1,0,0,
0,534,535,1,0,0,0,535,536,1,0,0,0,536,573,5,6,0,0,537,573,5,96,0,0,538,573,
5,97,0,0,539,573,5,98,0,0,540,573,5,44,0,0,541,573,5,45,0,0,542,573,5,46,
0,0,543,573,5,95,0,0,544,545,5,23,0,0,545,547,5,3,0,0,546,548,3,58,29,0,
547,546,1,0,0,0,547,548,1,0,0,0,548,549,1,0,0,0,549,573,5,4,0,0,550,573,
3,56,28,0,551,552,5,43,0,0,552,553,3,2,1,0,553,555,5,3,0,0,554,556,3,58,
29,0,555,554,1,0,0,0,555,556,1,0,0,0,556,557,1,0,0,0,557,559,5,4,0,0,558,
560,3,54,27,0,559,558,1,0,0,0,559,560,1,0,0,0,560,573,1,0,0,0,561,562,5,
32,0,0,562,564,5,3,0,0,563,565,3,44,22,0,564,563,1,0,0,0,564,565,1,0,0,0,
565,566,1,0,0,0,566,569,5,4,0,0,567,568,5,85,0,0,568,570,3,66,33,0,569,567,
1,0,0,0,569,570,1,0,0,0,570,571,1,0,0,0,571,573,3,48,24,0,572,517,1,0,0,
0,572,522,1,0,0,0,572,527,1,0,0,0,572,532,1,0,0,0,572,537,1,0,0,0,572,538,
1,0,0,0,572,539,1,0,0,0,572,540,1,0,0,0,572,541,1,0,0,0,572,542,1,0,0,0,
572,543,1,0,0,0,572,544,1,0,0,0,572,550,1,0,0,0,572,551,1,0,0,0,572,561,
1,0,0,0,573,585,1,0,0,0,574,575,10,4,0,0,575,577,5,3,0,0,576,578,3,58,29,
0,577,576,1,0,0,0,577,578,1,0,0,0,578,579,1,0,0,0,579,584,5,4,0,0,580,581,
10,3,0,0,581,582,5,1,0,0,582,584,5,95,0,0,583,574,1,0,0,0,583,580,1,0,0,
0,584,587,1,0,0,0,585,583,1,0,0,0,585,586,1,0,0,0,586,53,1,0,0,0,587,585,
1,0,0,0,588,592,5,5,0,0,589,591,3,20,10,0,590,589,1,0,0,0,591,594,1,0,0,
0,592,590,1,0,0,0,592,593,1,0,0,0,593,595,1,0,0,0,594,592,1,0,0,0,595,596,
5,6,0,0,596,55,1,0,0,0,597,598,5,26,0,0,598,600,5,3,0,0,599,601,3,58,29,
0,600,599,1,0,0,0,600,601,1,0,0,0,601,602,1,0,0,0,602,603,5,4,0,0,603,57,
1,0,0,0,604,609,3,50,25,0,605,606,5,9,0,0,606,608,3,50,25,0,607,605,1,0,
0,0,608,611,1,0,0,0,609,607,1,0,0,0,609,610,1,0,0,0,610,59,1,0,0,0,611,609,
1,0,0,0,612,617,3,62,31,0,613,614,5,9,0,0,614,616,3,62,31,0,615,613,1,0,
0,0,616,619,1,0,0,0,617,615,1,0,0,0,617,618,1,0,0,0,618,621,1,0,0,0,619,
617,1,0,0,0,620,622,5,9,0,0,621,620,1,0,0,0,621,622,1,0,0,0,622,61,1,0,0,
0,623,624,3,64,32,0,624,625,5,7,0,0,625,626,3,50,25,0,626,63,1,0,0,0,627,
635,5,98,0,0,628,635,5,95,0,0,629,635,5,96,0,0,630,631,5,3,0,0,631,632,3,
50,25,0,632,633,5,4,0,0,633,635,1,0,0,0,634,627,1,0,0,0,634,628,1,0,0,0,
634,629,1,0,0,0,634,630,1,0,0,0,635,65,1,0,0,0,636,637,6,33,-1,0,637,646,
5,50,0,0,638,646,5,51,0,0,639,646,5,52,0,0,640,646,5,53,0,0,641,646,5,54,
0,0,642,646,5,55,0,0,643,646,5,56,0,0,644,646,3,2,1,0,645,636,1,0,0,0,645,
638,1,0,0,0,645,639,1,0,0,0,645,640,1,0,0,0,645,641,1,0,0,0,645,642,1,0,
0,0,645,643,1,0,0,0,645,644,1,0,0,0,646,651,1,0,0,0,647,648,10,1,0,0,648,
650,5,15,0,0,649,647,1,0,0,0,650,653,1,0,0,0,651,649,1,0,0,0,651,652,1,0,
0,0,652,67,1,0,0,0,653,651,1,0,0,0,79,71,81,96,106,117,121,125,141,145,165,
173,181,189,197,206,210,216,223,229,235,240,244,247,251,258,267,275,286,
291,299,304,311,316,322,326,332,335,340,346,351,356,363,368,372,382,387,
398,404,411,413,417,428,437,439,445,461,510,512,514,524,529,534,547,555,
559,564,569,572,577,583,585,592,600,609,617,621,634,645,651];


const atn = new antlr4.atn.ATNDeserializer().deserialize(serializedATN);

const decisionsToDFA = atn.decisionToState.map( (ds, index) => new antlr4.dfa.DFA(ds, index) );

const sharedContextCache = new antlr4.atn.PredictionContextCache();

export default class TzdLangParser extends antlr4.Parser {

    static grammarFileName = "TzdLang.g4";
    static literalNames = [ null, "'.'", "';'", "'('", "')'", "'{'", "'}'", 
                            "':'", "'@'", "','", "'type'", "'dll'", "'prototype'", 
                            "'['", "']'", "'[]'", "'var'", "'const'", "'let'", 
                            "'static'", "'abstract'", "'enum'", "'in'", 
                            "'super'", "'native'", "'import'", null, "'class'", 
                            "'extends'", "'public'", "'private'", "'protected'", 
                            "'fun'", null, "'if'", "'else'", "'while'", 
                            "'for'", "'break'", "'continue'", "'switch'", 
                            "'case'", "'default'", "'new'", "'true'", "'false'", 
                            "'null'", "'throw'", "'try'", "'catch'", "'int'", 
                            "'float'", "'string'", "'bool'", "'void'", null, 
                            null, "'++'", "'--'", "'gxxx'", "'>>>='", "'>>='", 
                            "'<<='", "'>>>'", "'>>'", "'<<'", "'&='", "'|='", 
                            "'^='", "'%='", "'+='", "'-='", "'*='", "'/='", 
                            "'=='", "'!='", "'>='", "'<='", "'&&'", "'||'", 
                            "'**'", "'~'", "'&'", "'|'", "'^'", "'->'", 
                            "'+'", "'-'", "'*'", "'/'", "'%'", "'!'", "'>'", 
                            "'<'", "'='" ];
    static symbolicNames = [ null, null, null, null, null, null, null, null, 
                             null, null, null, null, null, null, null, null, 
                             "KW_VAR", "KW_CONST", "KW_LET", "KW_STATIC", 
                             "KW_ABSTRACT", "KW_ENUM", "KW_IN", "KW_SUPER", 
                             "KW_NATIVE", "KW_IMPORT", "KW_PRINT", "KW_CLASS", 
                             "KW_EXTENDS", "KW_PUBLIC", "KW_PRIVATE", "KW_PROTECTED", 
                             "KW_FUN", "KW_RET", "KW_IF", "KW_ELSE", "KW_WHILE", 
                             "KW_FOR", "KW_BREAK", "KW_CONTINUE", "KW_SWITCH", 
                             "KW_CASE", "KW_DEFAULT", "KW_NEW", "KW_TRUE", 
                             "KW_FALSE", "KW_NULL", "KW_THROW", "KW_TRY", 
                             "KW_CATCH", "T_INT", "T_FLOAT", "T_STRING", 
                             "T_BOOL", "T_VOID", "T_PTR", "T_FUNCTION", 
                             "INC", "DEC", "GXXX", "USHR_ASSIGN", "SHR_ASSIGN", 
                             "SHL_ASSIGN", "USHR", "SHR", "SHL", "AND_ASSIGN", 
                             "OR_ASSIGN", "XOR_ASSIGN", "MOD_ASSIGN", "PLUS_ASSIGN", 
                             "MIN_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN", "EEQ", 
                             "NEQ", "GE", "LE", "AND", "OR", "POW", "BIT_NOT", 
                             "BIT_AND", "BIT_OR", "BIT_XOR", "ARROW", "PLUS", 
                             "MINUS", "MUL", "DIV", "MOD", "NOT", "GT", 
                             "LT", "ASSIGN", "IDENTIFIER", "INTEGER", "FLOAT", 
                             "STRING", "LINE_COMMENT", "BLOCK_COMMENT", 
                             "WS" ];
    static ruleNames = [ "program", "qualifiedName", "statement", "switchCase", 
                         "switchDefault", "annotationDeclaration", "enumDeclaration", 
                         "enumList", "classDeclaration", "classBody", "classMember", 
                         "memberDecl", "annotationUsage", "accessModifier", 
                         "functionDeclaration", "nativeFunctionDeclaration", 
                         "nativeAttrList", "nativeAttr", "nativePropKey", 
                         "variableDeclaration", "forInit", "importStatement", 
                         "paramList", "param", "block", "expression", "atom", 
                         "classOverrideBlock", "printFunction", "exprList", 
                         "mapEntryList", "mapEntry", "mapKey", "typeType" ];

    constructor(input) {
        super(input);
        this._interp = new antlr4.atn.ParserATNSimulator(this, atn, decisionsToDFA, sharedContextCache);
        this.ruleNames = TzdLangParser.ruleNames;
        this.literalNames = TzdLangParser.literalNames;
        this.symbolicNames = TzdLangParser.symbolicNames;
    }

    sempred(localctx, ruleIndex, predIndex) {
    	switch(ruleIndex) {
    	case 25:
    	    		return this.expression_sempred(localctx, predIndex);
    	case 26:
    	    		return this.atom_sempred(localctx, predIndex);
    	case 33:
    	    		return this.typeType_sempred(localctx, predIndex);
        default:
            throw "No predicate with index:" + ruleIndex;
       }
    }

    expression_sempred(localctx, predIndex) {
    	switch(predIndex) {
    		case 0:
    			return this.precpred(this._ctx, 16);
    		case 1:
    			return this.precpred(this._ctx, 12);
    		case 2:
    			return this.precpred(this._ctx, 11);
    		case 3:
    			return this.precpred(this._ctx, 10);
    		case 4:
    			return this.precpred(this._ctx, 9);
    		case 5:
    			return this.precpred(this._ctx, 8);
    		case 6:
    			return this.precpred(this._ctx, 7);
    		case 7:
    			return this.precpred(this._ctx, 6);
    		case 8:
    			return this.precpred(this._ctx, 5);
    		case 9:
    			return this.precpred(this._ctx, 4);
    		case 10:
    			return this.precpred(this._ctx, 3);
    		case 11:
    			return this.precpred(this._ctx, 2);
    		case 12:
    			return this.precpred(this._ctx, 18);
    		case 13:
    			return this.precpred(this._ctx, 17);
    		case 14:
    			return this.precpred(this._ctx, 13);
    		default:
    			throw "No predicate with index:" + predIndex;
    	}
    };

    atom_sempred(localctx, predIndex) {
    	switch(predIndex) {
    		case 15:
    			return this.precpred(this._ctx, 4);
    		case 16:
    			return this.precpred(this._ctx, 3);
    		default:
    			throw "No predicate with index:" + predIndex;
    	}
    };

    typeType_sempred(localctx, predIndex) {
    	switch(predIndex) {
    		case 17:
    			return this.precpred(this._ctx, 1);
    		default:
    			throw "No predicate with index:" + predIndex;
    	}
    };




	program() {
	    let localctx = new ProgramContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 0, TzdLangParser.RULE_program);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 71;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 262218028) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 268302839) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	            this.state = 68;
	            this.statement();
	            this.state = 73;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	        this.state = 74;
	        this.match(TzdLangParser.EOF);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	qualifiedName() {
	    let localctx = new QualifiedNameContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 2, TzdLangParser.RULE_qualifiedName);
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 76;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 81;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,1,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                this.state = 77;
	                this.match(TzdLangParser.T__0);
	                this.state = 78;
	                this.match(TzdLangParser.IDENTIFIER); 
	            }
	            this.state = 83;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,1,this._ctx);
	        }

	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	statement() {
	    let localctx = new StatementContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 4, TzdLangParser.RULE_statement);
	    var _la = 0;
	    try {
	        this.state = 165;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,9,this._ctx);
	        switch(la_) {
	        case 1:
	            localctx = new BlockStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 1);
	            this.state = 84;
	            this.block();
	            break;

	        case 2:
	            localctx = new ClassDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 2);
	            this.state = 85;
	            this.classDeclaration();
	            break;

	        case 3:
	            localctx = new AnnotationDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 3);
	            this.state = 86;
	            this.annotationDeclaration();
	            break;

	        case 4:
	            localctx = new EnumDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 4);
	            this.state = 87;
	            this.enumDeclaration();
	            break;

	        case 5:
	            localctx = new FunDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 5);
	            this.state = 88;
	            this.functionDeclaration();
	            break;

	        case 6:
	            localctx = new NativeFunDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 6);
	            this.state = 89;
	            this.nativeFunctionDeclaration();
	            break;

	        case 7:
	            localctx = new VarDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 7);
	            this.state = 90;
	            this.variableDeclaration();
	            this.state = 91;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 8:
	            localctx = new ImportStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 8);
	            this.state = 93;
	            this.importStatement();
	            break;

	        case 9:
	            localctx = new ReturnStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 9);
	            this.state = 94;
	            this.match(TzdLangParser.KW_RET);
	            this.state = 96;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 95;
	                this.expression(0);
	            }

	            this.state = 98;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 10:
	            localctx = new IfStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 10);
	            this.state = 99;
	            this.match(TzdLangParser.KW_IF);
	            this.state = 100;
	            this.match(TzdLangParser.T__2);
	            this.state = 101;
	            this.expression(0);
	            this.state = 102;
	            this.match(TzdLangParser.T__3);
	            this.state = 103;
	            this.statement();
	            this.state = 106;
	            this._errHandler.sync(this);
	            var la_ = this._interp.adaptivePredict(this._input,3,this._ctx);
	            if(la_===1) {
	                this.state = 104;
	                this.match(TzdLangParser.KW_ELSE);
	                this.state = 105;
	                this.statement();

	            }
	            break;

	        case 11:
	            localctx = new WhileStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 11);
	            this.state = 108;
	            this.match(TzdLangParser.KW_WHILE);
	            this.state = 109;
	            this.match(TzdLangParser.T__2);
	            this.state = 110;
	            this.expression(0);
	            this.state = 111;
	            this.match(TzdLangParser.T__3);
	            this.state = 112;
	            this.statement();
	            break;

	        case 12:
	            localctx = new ForStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 12);
	            this.state = 114;
	            this.match(TzdLangParser.KW_FOR);
	            this.state = 115;
	            this.match(TzdLangParser.T__2);
	            this.state = 117;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75571240) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 268204033) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 116;
	                this.forInit();
	            }

	            this.state = 119;
	            this.match(TzdLangParser.T__1);
	            this.state = 121;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 120;
	                localctx.cond = this.expression(0);
	            }

	            this.state = 123;
	            this.match(TzdLangParser.T__1);
	            this.state = 125;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 124;
	                localctx.step = this.expression(0);
	            }

	            this.state = 127;
	            this.match(TzdLangParser.T__3);
	            this.state = 128;
	            this.statement();
	            break;

	        case 13:
	            localctx = new BreakStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 13);
	            this.state = 129;
	            this.match(TzdLangParser.KW_BREAK);
	            this.state = 130;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 14:
	            localctx = new ContinueStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 14);
	            this.state = 131;
	            this.match(TzdLangParser.KW_CONTINUE);
	            this.state = 132;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 15:
	            localctx = new SwitchStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 15);
	            this.state = 133;
	            this.match(TzdLangParser.KW_SWITCH);
	            this.state = 134;
	            this.match(TzdLangParser.T__2);
	            this.state = 135;
	            this.expression(0);
	            this.state = 136;
	            this.match(TzdLangParser.T__3);
	            this.state = 137;
	            this.match(TzdLangParser.T__4);
	            this.state = 141;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            while(_la===41) {
	                this.state = 138;
	                this.switchCase();
	                this.state = 143;
	                this._errHandler.sync(this);
	                _la = this._input.LA(1);
	            }
	            this.state = 145;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===42) {
	                this.state = 144;
	                this.switchDefault();
	            }

	            this.state = 147;
	            this.match(TzdLangParser.T__5);
	            break;

	        case 16:
	            localctx = new TryCatchStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 16);
	            this.state = 149;
	            this.match(TzdLangParser.KW_TRY);
	            this.state = 150;
	            this.block();
	            this.state = 151;
	            this.match(TzdLangParser.KW_CATCH);
	            this.state = 152;
	            this.match(TzdLangParser.T__2);
	            this.state = 153;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 154;
	            this.match(TzdLangParser.T__3);
	            this.state = 155;
	            this.block();
	            break;

	        case 17:
	            localctx = new ThrowStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 17);
	            this.state = 157;
	            this.match(TzdLangParser.KW_THROW);
	            this.state = 158;
	            this.expression(0);
	            this.state = 159;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 18:
	            localctx = new ExprStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 18);
	            this.state = 161;
	            this.expression(0);
	            this.state = 162;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 19:
	            localctx = new EmptyStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 19);
	            this.state = 164;
	            this.match(TzdLangParser.T__1);
	            break;

	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	switchCase() {
	    let localctx = new SwitchCaseContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 6, TzdLangParser.RULE_switchCase);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 167;
	        this.match(TzdLangParser.KW_CASE);
	        this.state = 168;
	        this.expression(0);
	        this.state = 169;
	        this.match(TzdLangParser.T__6);
	        this.state = 173;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 262218028) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 268302839) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	            this.state = 170;
	            this.statement();
	            this.state = 175;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	switchDefault() {
	    let localctx = new SwitchDefaultContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 8, TzdLangParser.RULE_switchDefault);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 176;
	        this.match(TzdLangParser.KW_DEFAULT);
	        this.state = 177;
	        this.match(TzdLangParser.T__6);
	        this.state = 181;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 262218028) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 268302839) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	            this.state = 178;
	            this.statement();
	            this.state = 183;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	annotationDeclaration() {
	    let localctx = new AnnotationDeclarationContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 10, TzdLangParser.RULE_annotationDeclaration);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 184;
	        this.match(TzdLangParser.KW_CLASS);
	        this.state = 185;
	        this.match(TzdLangParser.T__7);
	        this.state = 186;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 187;
	        this.match(TzdLangParser.T__2);
	        this.state = 189;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	            this.state = 188;
	            this.paramList();
	        }

	        this.state = 191;
	        this.match(TzdLangParser.T__3);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	enumDeclaration() {
	    let localctx = new EnumDeclarationContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 12, TzdLangParser.RULE_enumDeclaration);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 193;
	        this.match(TzdLangParser.KW_ENUM);
	        this.state = 194;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 195;
	        this.match(TzdLangParser.T__4);
	        this.state = 197;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===95) {
	            this.state = 196;
	            this.enumList();
	        }

	        this.state = 199;
	        this.match(TzdLangParser.T__5);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	enumList() {
	    let localctx = new EnumListContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 14, TzdLangParser.RULE_enumList);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 201;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 206;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 202;
	            this.match(TzdLangParser.T__8);
	            this.state = 203;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 208;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	classDeclaration() {
	    let localctx = new ClassDeclarationContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 16, TzdLangParser.RULE_classDeclaration);
	    var _la = 0;
	    try {
	        this.state = 235;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,19,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 210;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===8) {
	                this.state = 209;
	                this.annotationUsage();
	            }

	            this.state = 212;
	            this.match(TzdLangParser.KW_CLASS);
	            this.state = 213;
	            this.qualifiedName();
	            this.state = 216;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===7) {
	                this.state = 214;
	                this.match(TzdLangParser.T__6);
	                this.state = 215;
	                this.qualifiedName();
	            }

	            this.state = 218;
	            this.match(TzdLangParser.T__4);
	            this.state = 219;
	            this.classBody();
	            this.state = 220;
	            this.match(TzdLangParser.T__5);
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 223;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===8) {
	                this.state = 222;
	                this.annotationUsage();
	            }

	            this.state = 225;
	            this.match(TzdLangParser.KW_CLASS);
	            this.state = 226;
	            this.qualifiedName();
	            this.state = 229;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===28) {
	                this.state = 227;
	                this.match(TzdLangParser.KW_EXTENDS);
	                this.state = 228;
	                this.qualifiedName();
	            }

	            this.state = 231;
	            this.match(TzdLangParser.T__4);
	            this.state = 232;
	            this.classBody();
	            this.state = 233;
	            this.match(TzdLangParser.T__5);
	            break;

	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	classBody() {
	    let localctx = new ClassBodyContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 18, TzdLangParser.RULE_classBody);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 240;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(((((_la - 8)) & ~0x1f) === 0 && ((1 << (_la - 8)) & 31530753) !== 0) || _la===95) {
	            this.state = 237;
	            this.classMember();
	            this.state = 242;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	classMember() {
	    let localctx = new ClassMemberContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 20, TzdLangParser.RULE_classMember);
	    var _la = 0;
	    try {
	        this.state = 251;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,23,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 244;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===8) {
	                this.state = 243;
	                this.annotationUsage();
	            }

	            this.state = 247;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 3758096384) !== 0)) {
	                this.state = 246;
	                this.accessModifier();
	            }

	            this.state = 249;
	            this.memberDecl();
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 250;
	            this.nativeFunctionDeclaration();
	            break;

	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	memberDecl() {
	    let localctx = new MemberDeclContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 22, TzdLangParser.RULE_memberDecl);
	    var _la = 0;
	    try {
	        this.state = 326;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 16:
	            localctx = new FieldVarDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 1);
	            this.state = 253;
	            this.match(TzdLangParser.KW_VAR);
	            this.state = 254;
	            this.typeType(0);
	            this.state = 255;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 258;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===94) {
	                this.state = 256;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 257;
	                this.expression(0);
	            }

	            this.state = 260;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 18:
	            localctx = new FieldLetDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 2);
	            this.state = 262;
	            this.match(TzdLangParser.KW_LET);
	            this.state = 263;
	            this.typeType(0);
	            this.state = 264;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 267;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===94) {
	                this.state = 265;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 266;
	                this.expression(0);
	            }

	            this.state = 269;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 17:
	            localctx = new FieldConstDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 3);
	            this.state = 271;
	            this.match(TzdLangParser.KW_CONST);
	            this.state = 272;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 275;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===7) {
	                this.state = 273;
	                this.match(TzdLangParser.T__6);
	                this.state = 274;
	                this.typeType(0);
	            }

	            this.state = 277;
	            this.match(TzdLangParser.ASSIGN);
	            this.state = 278;
	            this.expression(0);
	            this.state = 279;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 19:
	            localctx = new MethodStaticDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 4);
	            this.state = 281;
	            this.match(TzdLangParser.KW_STATIC);
	            this.state = 282;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 283;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 284;
	            this.match(TzdLangParser.T__2);
	            this.state = 286;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	                this.state = 285;
	                this.paramList();
	            }

	            this.state = 288;
	            this.match(TzdLangParser.T__3);
	            this.state = 291;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===85) {
	                this.state = 289;
	                this.match(TzdLangParser.ARROW);
	                this.state = 290;
	                this.typeType(0);
	            }

	            this.state = 293;
	            this.block();
	            break;
	        case 20:
	            localctx = new MethodAbstractDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 5);
	            this.state = 294;
	            this.match(TzdLangParser.KW_ABSTRACT);
	            this.state = 295;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 296;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 297;
	            this.match(TzdLangParser.T__2);
	            this.state = 299;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	                this.state = 298;
	                this.paramList();
	            }

	            this.state = 301;
	            this.match(TzdLangParser.T__3);
	            this.state = 304;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===85) {
	                this.state = 302;
	                this.match(TzdLangParser.ARROW);
	                this.state = 303;
	                this.typeType(0);
	            }

	            this.state = 306;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 32:
	            localctx = new MethodDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 6);
	            this.state = 307;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 308;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 309;
	            this.match(TzdLangParser.T__2);
	            this.state = 311;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	                this.state = 310;
	                this.paramList();
	            }

	            this.state = 313;
	            this.match(TzdLangParser.T__3);
	            this.state = 316;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===85) {
	                this.state = 314;
	                this.match(TzdLangParser.ARROW);
	                this.state = 315;
	                this.typeType(0);
	            }

	            this.state = 318;
	            this.block();
	            break;
	        case 95:
	            localctx = new ConstructorDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 7);
	            this.state = 319;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 320;
	            this.match(TzdLangParser.T__2);
	            this.state = 322;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	                this.state = 321;
	                this.paramList();
	            }

	            this.state = 324;
	            this.match(TzdLangParser.T__3);
	            this.state = 325;
	            this.block();
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	annotationUsage() {
	    let localctx = new AnnotationUsageContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 24, TzdLangParser.RULE_annotationUsage);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 328;
	        this.match(TzdLangParser.T__7);
	        this.state = 329;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 335;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===3) {
	            this.state = 330;
	            this.match(TzdLangParser.T__2);
	            this.state = 332;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 331;
	                this.exprList();
	            }

	            this.state = 334;
	            this.match(TzdLangParser.T__3);
	        }

	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	accessModifier() {
	    let localctx = new AccessModifierContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 26, TzdLangParser.RULE_accessModifier);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 337;
	        _la = this._input.LA(1);
	        if(!((((_la) & ~0x1f) === 0 && ((1 << _la) & 3758096384) !== 0))) {
	        this._errHandler.recoverInline(this);
	        }
	        else {
	        	this._errHandler.reportMatch(this);
	            this.consume();
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	functionDeclaration() {
	    let localctx = new FunctionDeclarationContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 28, TzdLangParser.RULE_functionDeclaration);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 340;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===8) {
	            this.state = 339;
	            this.annotationUsage();
	        }

	        this.state = 342;
	        this.match(TzdLangParser.KW_FUN);
	        this.state = 343;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 344;
	        this.match(TzdLangParser.T__2);
	        this.state = 346;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	            this.state = 345;
	            this.paramList();
	        }

	        this.state = 348;
	        this.match(TzdLangParser.T__3);
	        this.state = 351;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===85) {
	            this.state = 349;
	            this.match(TzdLangParser.ARROW);
	            this.state = 350;
	            this.typeType(0);
	        }

	        this.state = 353;
	        this.block();
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	nativeFunctionDeclaration() {
	    let localctx = new NativeFunctionDeclarationContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 30, TzdLangParser.RULE_nativeFunctionDeclaration);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 356;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===8) {
	            this.state = 355;
	            this.annotationUsage();
	        }

	        this.state = 358;
	        this.match(TzdLangParser.KW_NATIVE);
	        this.state = 359;
	        this.match(TzdLangParser.KW_FUN);
	        this.state = 360;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 361;
	        this.match(TzdLangParser.T__2);
	        this.state = 363;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	            this.state = 362;
	            this.paramList();
	        }

	        this.state = 365;
	        this.match(TzdLangParser.T__3);
	        this.state = 368;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===85) {
	            this.state = 366;
	            this.match(TzdLangParser.ARROW);
	            this.state = 367;
	            this.typeType(0);
	        }

	        this.state = 370;
	        this.match(TzdLangParser.T__2);
	        this.state = 372;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if((((_la) & ~0x1f) === 0 && ((1 << _la) & 7168) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 33292291) !== 0) || _la===95) {
	            this.state = 371;
	            this.nativeAttrList();
	        }

	        this.state = 374;
	        this.match(TzdLangParser.T__3);
	        this.state = 375;
	        this.match(TzdLangParser.T__1);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	nativeAttrList() {
	    let localctx = new NativeAttrListContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 32, TzdLangParser.RULE_nativeAttrList);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 377;
	        this.nativeAttr();
	        this.state = 382;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 378;
	            this.match(TzdLangParser.T__8);
	            this.state = 379;
	            this.nativeAttr();
	            this.state = 384;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	nativeAttr() {
	    let localctx = new NativeAttrContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 34, TzdLangParser.RULE_nativeAttr);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 387;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 95:
	            this.state = 385;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;
	        case 10:
	        case 11:
	        case 12:
	        case 32:
	        case 33:
	        case 50:
	        case 51:
	        case 52:
	        case 53:
	        case 54:
	        case 55:
	        case 56:
	            this.state = 386;
	            this.nativePropKey();
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	        this.state = 389;
	        this.match(TzdLangParser.ASSIGN);
	        this.state = 390;
	        _la = this._input.LA(1);
	        if(!(_la===96 || _la===98)) {
	        this._errHandler.recoverInline(this);
	        }
	        else {
	        	this._errHandler.reportMatch(this);
	            this.consume();
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	nativePropKey() {
	    let localctx = new NativePropKeyContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 36, TzdLangParser.RULE_nativePropKey);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 392;
	        _la = this._input.LA(1);
	        if(!((((_la) & ~0x1f) === 0 && ((1 << _la) & 7168) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 33292291) !== 0))) {
	        this._errHandler.recoverInline(this);
	        }
	        else {
	        	this._errHandler.reportMatch(this);
	            this.consume();
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	variableDeclaration() {
	    let localctx = new VariableDeclarationContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 38, TzdLangParser.RULE_variableDeclaration);
	    var _la = 0;
	    try {
	        this.state = 413;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,49,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 394;
	            this.typeType(0);
	            this.state = 395;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 398;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===94) {
	                this.state = 396;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 397;
	                this.expression(0);
	            }

	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 400;
	            this.match(TzdLangParser.KW_VAR);
	            this.state = 401;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 404;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===94) {
	                this.state = 402;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 403;
	                this.expression(0);
	            }

	            break;

	        case 3:
	            this.enterOuterAlt(localctx, 3);
	            this.state = 406;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 407;
	            this.match(TzdLangParser.T__6);
	            this.state = 408;
	            this.typeType(0);
	            this.state = 411;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===94) {
	                this.state = 409;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 410;
	                this.expression(0);
	            }

	            break;

	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	forInit() {
	    let localctx = new ForInitContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 40, TzdLangParser.RULE_forInit);
	    try {
	        this.state = 417;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,50,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 415;
	            this.variableDeclaration();
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 416;
	            this.expression(0);
	            break;

	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	importStatement() {
	    let localctx = new ImportStatementContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 42, TzdLangParser.RULE_importStatement);
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 419;
	        this.match(TzdLangParser.KW_IMPORT);
	        this.state = 420;
	        this.match(TzdLangParser.STRING);
	        this.state = 421;
	        this.match(TzdLangParser.T__1);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	paramList() {
	    let localctx = new ParamListContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 44, TzdLangParser.RULE_paramList);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 423;
	        this.param();
	        this.state = 428;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 424;
	            this.match(TzdLangParser.T__8);
	            this.state = 425;
	            this.param();
	            this.state = 430;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	param() {
	    let localctx = new ParamContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 46, TzdLangParser.RULE_param);
	    var _la = 0;
	    try {
	        this.state = 439;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,53,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 431;
	            this.typeType(0);
	            this.state = 432;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 434;
	            _la = this._input.LA(1);
	            if(!(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95)) {
	            this._errHandler.recoverInline(this);
	            }
	            else {
	            	this._errHandler.reportMatch(this);
	                this.consume();
	            }
	            this.state = 437;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===7) {
	                this.state = 435;
	                this.match(TzdLangParser.T__6);
	                this.state = 436;
	                this.typeType(0);
	            }

	            break;

	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	block() {
	    let localctx = new BlockContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 48, TzdLangParser.RULE_block);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 441;
	        this.match(TzdLangParser.T__4);
	        this.state = 445;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 262218028) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 268302839) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	            this.state = 442;
	            this.statement();
	            this.state = 447;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	        this.state = 448;
	        this.match(TzdLangParser.T__5);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}


	expression(_p) {
		if(_p===undefined) {
		    _p = 0;
		}
	    const _parentctx = this._ctx;
	    const _parentState = this.state;
	    let localctx = new ExpressionContext(this, this._ctx, _parentState);
	    let _prevctx = localctx;
	    const _startState = 50;
	    this.enterRecursionRule(localctx, 50, TzdLangParser.RULE_expression, _p);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 461;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,55,this._ctx);
	        switch(la_) {
	        case 1:
	            localctx = new CastExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;

	            this.state = 451;
	            this.match(TzdLangParser.T__2);
	            this.state = 452;
	            this.typeType(0);
	            this.state = 453;
	            this.match(TzdLangParser.T__3);
	            this.state = 454;
	            this.expression(19);
	            break;

	        case 2:
	            localctx = new PrefixExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 456;
	            _la = this._input.LA(1);
	            if(!(_la===57 || _la===58)) {
	            this._errHandler.recoverInline(this);
	            }
	            else {
	            	this._errHandler.reportMatch(this);
	                this.consume();
	            }
	            this.state = 457;
	            this.expression(15);
	            break;

	        case 3:
	            localctx = new UnaryExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 458;
	            _la = this._input.LA(1);
	            if(!(((((_la - 59)) & ~0x1f) === 0 && ((1 << (_la - 59)) & 406847489) !== 0) || _la===91)) {
	            this._errHandler.recoverInline(this);
	            }
	            else {
	            	this._errHandler.reportMatch(this);
	                this.consume();
	            }
	            this.state = 459;
	            this.expression(14);
	            break;

	        case 4:
	            localctx = new AtomExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 460;
	            this.atom(0);
	            break;

	        }
	        this._ctx.stop = this._input.LT(-1);
	        this.state = 514;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,58,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                if(this._parseListeners!==null) {
	                    this.triggerExitRuleEvent();
	                }
	                _prevctx = localctx;
	                this.state = 512;
	                this._errHandler.sync(this);
	                var la_ = this._interp.adaptivePredict(this._input,57,this._ctx);
	                switch(la_) {
	                case 1:
	                    localctx = new PowerExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 463;
	                    if (!( this.precpred(this._ctx, 16))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 16)");
	                    }
	                    this.state = 464;
	                    this.match(TzdLangParser.POW);
	                    this.state = 465;
	                    this.expression(16);
	                    break;

	                case 2:
	                    localctx = new MultiplicativeExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 466;
	                    if (!( this.precpred(this._ctx, 12))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 12)");
	                    }
	                    this.state = 467;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 88)) & ~0x1f) === 0 && ((1 << (_la - 88)) & 7) !== 0))) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 468;
	                    this.expression(13);
	                    break;

	                case 3:
	                    localctx = new AdditiveExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 469;
	                    if (!( this.precpred(this._ctx, 11))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 11)");
	                    }
	                    this.state = 470;
	                    _la = this._input.LA(1);
	                    if(!(_la===86 || _la===87)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 471;
	                    this.expression(12);
	                    break;

	                case 4:
	                    localctx = new ShiftExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 472;
	                    if (!( this.precpred(this._ctx, 10))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 10)");
	                    }
	                    this.state = 473;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 63)) & ~0x1f) === 0 && ((1 << (_la - 63)) & 7) !== 0))) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 474;
	                    this.expression(11);
	                    break;

	                case 5:
	                    localctx = new RelationalExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 475;
	                    if (!( this.precpred(this._ctx, 9))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 9)");
	                    }
	                    this.state = 476;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 76)) & ~0x1f) === 0 && ((1 << (_la - 76)) & 196611) !== 0))) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 477;
	                    this.expression(10);
	                    break;

	                case 6:
	                    localctx = new EqualityExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 478;
	                    if (!( this.precpred(this._ctx, 8))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 8)");
	                    }
	                    this.state = 479;
	                    _la = this._input.LA(1);
	                    if(!(_la===74 || _la===75)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 480;
	                    this.expression(9);
	                    break;

	                case 7:
	                    localctx = new BitAndExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 481;
	                    if (!( this.precpred(this._ctx, 7))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 7)");
	                    }
	                    this.state = 482;
	                    this.match(TzdLangParser.BIT_AND);
	                    this.state = 483;
	                    this.expression(8);
	                    break;

	                case 8:
	                    localctx = new BitXorExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 484;
	                    if (!( this.precpred(this._ctx, 6))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 6)");
	                    }
	                    this.state = 485;
	                    this.match(TzdLangParser.BIT_XOR);
	                    this.state = 486;
	                    this.expression(7);
	                    break;

	                case 9:
	                    localctx = new BitOrExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 487;
	                    if (!( this.precpred(this._ctx, 5))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 5)");
	                    }
	                    this.state = 488;
	                    this.match(TzdLangParser.BIT_OR);
	                    this.state = 489;
	                    this.expression(6);
	                    break;

	                case 10:
	                    localctx = new LogicalAndExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 490;
	                    if (!( this.precpred(this._ctx, 4))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 4)");
	                    }
	                    this.state = 491;
	                    this.match(TzdLangParser.AND);
	                    this.state = 492;
	                    this.expression(5);
	                    break;

	                case 11:
	                    localctx = new LogicalOrExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 493;
	                    if (!( this.precpred(this._ctx, 3))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 3)");
	                    }
	                    this.state = 494;
	                    this.match(TzdLangParser.OR);
	                    this.state = 495;
	                    this.expression(4);
	                    break;

	                case 12:
	                    localctx = new AssignmentExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 496;
	                    if (!( this.precpred(this._ctx, 2))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 2)");
	                    }
	                    this.state = 497;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 60)) & ~0x1f) === 0 && ((1 << (_la - 60)) & 16327) !== 0) || _la===94)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 498;
	                    this.expression(2);
	                    break;

	                case 13:
	                    localctx = new IndexExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 499;
	                    if (!( this.precpred(this._ctx, 18))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 18)");
	                    }
	                    this.state = 500;
	                    this.match(TzdLangParser.T__12);
	                    this.state = 501;
	                    this.expression(0);
	                    this.state = 502;
	                    this.match(TzdLangParser.T__13);
	                    break;

	                case 14:
	                    localctx = new PostfixExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 504;
	                    if (!( this.precpred(this._ctx, 17))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 17)");
	                    }
	                    this.state = 505;
	                    _la = this._input.LA(1);
	                    if(!(_la===57 || _la===58)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    break;

	                case 15:
	                    localctx = new TypeCheckExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 506;
	                    if (!( this.precpred(this._ctx, 13))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 13)");
	                    }
	                    this.state = 507;
	                    this.match(TzdLangParser.KW_IN);
	                    this.state = 510;
	                    this._errHandler.sync(this);
	                    var la_ = this._interp.adaptivePredict(this._input,56,this._ctx);
	                    switch(la_) {
	                    case 1:
	                        this.state = 508;
	                        this.qualifiedName();
	                        break;

	                    case 2:
	                        this.state = 509;
	                        this.typeType(0);
	                        break;

	                    }
	                    break;

	                } 
	            }
	            this.state = 516;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,58,this._ctx);
	        }

	    } catch( error) {
	        if(error instanceof antlr4.error.RecognitionException) {
		        localctx.exception = error;
		        this._errHandler.reportError(this, error);
		        this._errHandler.recover(this, error);
		    } else {
		    	throw error;
		    }
	    } finally {
	        this.unrollRecursionContexts(_parentctx)
	    }
	    return localctx;
	}


	atom(_p) {
		if(_p===undefined) {
		    _p = 0;
		}
	    const _parentctx = this._ctx;
	    const _parentState = this.state;
	    let localctx = new AtomContext(this, this._ctx, _parentState);
	    let _prevctx = localctx;
	    const _startState = 52;
	    this.enterRecursionRule(localctx, 52, TzdLangParser.RULE_atom, _p);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 572;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,67,this._ctx);
	        switch(la_) {
	        case 1:
	            localctx = new ParenExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;

	            this.state = 518;
	            this.match(TzdLangParser.T__2);
	            this.state = 519;
	            this.expression(0);
	            this.state = 520;
	            this.match(TzdLangParser.T__3);
	            break;

	        case 2:
	            localctx = new ArrayLiteralExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 522;
	            this.match(TzdLangParser.T__12);
	            this.state = 524;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 523;
	                this.exprList();
	            }

	            this.state = 526;
	            this.match(TzdLangParser.T__13);
	            break;

	        case 3:
	            localctx = new MapLiteralExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 527;
	            this.match(TzdLangParser.T__4);
	            this.state = 529;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===3 || ((((_la - 95)) & ~0x1f) === 0 && ((1 << (_la - 95)) & 11) !== 0)) {
	                this.state = 528;
	                this.mapEntryList();
	            }

	            this.state = 531;
	            this.match(TzdLangParser.T__5);
	            break;

	        case 4:
	            localctx = new ArrayLiteralExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 532;
	            this.match(TzdLangParser.T__4);
	            this.state = 534;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 533;
	                this.exprList();
	            }

	            this.state = 536;
	            this.match(TzdLangParser.T__5);
	            break;

	        case 5:
	            localctx = new IntExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 537;
	            this.match(TzdLangParser.INTEGER);
	            break;

	        case 6:
	            localctx = new FloatExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 538;
	            this.match(TzdLangParser.FLOAT);
	            break;

	        case 7:
	            localctx = new StringExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 539;
	            this.match(TzdLangParser.STRING);
	            break;

	        case 8:
	            localctx = new BoolTrueExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 540;
	            this.match(TzdLangParser.KW_TRUE);
	            break;

	        case 9:
	            localctx = new BoolFalseExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 541;
	            this.match(TzdLangParser.KW_FALSE);
	            break;

	        case 10:
	            localctx = new NullExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 542;
	            this.match(TzdLangParser.KW_NULL);
	            break;

	        case 11:
	            localctx = new IdExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 543;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;

	        case 12:
	            localctx = new SuperExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 544;
	            this.match(TzdLangParser.KW_SUPER);
	            this.state = 545;
	            this.match(TzdLangParser.T__2);
	            this.state = 547;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 546;
	                this.exprList();
	            }

	            this.state = 549;
	            this.match(TzdLangParser.T__3);
	            break;

	        case 13:
	            localctx = new PrintFunExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 550;
	            this.printFunction();
	            break;

	        case 14:
	            localctx = new NewExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 551;
	            this.match(TzdLangParser.KW_NEW);
	            this.state = 552;
	            this.qualifiedName();
	            this.state = 553;
	            this.match(TzdLangParser.T__2);
	            this.state = 555;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                this.state = 554;
	                this.exprList();
	            }

	            this.state = 557;
	            this.match(TzdLangParser.T__3);
	            this.state = 559;
	            this._errHandler.sync(this);
	            var la_ = this._interp.adaptivePredict(this._input,64,this._ctx);
	            if(la_===1) {
	                this.state = 558;
	                this.classOverrideBlock();

	            }
	            break;

	        case 15:
	            localctx = new LambdaExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 561;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 562;
	            this.match(TzdLangParser.T__2);
	            this.state = 564;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 16646145) !== 0) || _la===95) {
	                this.state = 563;
	                this.paramList();
	            }

	            this.state = 566;
	            this.match(TzdLangParser.T__3);
	            this.state = 569;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===85) {
	                this.state = 567;
	                this.match(TzdLangParser.ARROW);
	                this.state = 568;
	                this.typeType(0);
	            }

	            this.state = 571;
	            this.block();
	            break;

	        }
	        this._ctx.stop = this._input.LT(-1);
	        this.state = 585;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,70,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                if(this._parseListeners!==null) {
	                    this.triggerExitRuleEvent();
	                }
	                _prevctx = localctx;
	                this.state = 583;
	                this._errHandler.sync(this);
	                var la_ = this._interp.adaptivePredict(this._input,69,this._ctx);
	                switch(la_) {
	                case 1:
	                    localctx = new CallExprContext(this, new AtomContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_atom);
	                    this.state = 574;
	                    if (!( this.precpred(this._ctx, 4))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 4)");
	                    }
	                    this.state = 575;
	                    this.match(TzdLangParser.T__2);
	                    this.state = 577;
	                    this._errHandler.sync(this);
	                    _la = this._input.LA(1);
	                    if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	                        this.state = 576;
	                        this.exprList();
	                    }

	                    this.state = 579;
	                    this.match(TzdLangParser.T__3);
	                    break;

	                case 2:
	                    localctx = new MemberAccessExprContext(this, new AtomContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_atom);
	                    this.state = 580;
	                    if (!( this.precpred(this._ctx, 3))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 3)");
	                    }
	                    this.state = 581;
	                    this.match(TzdLangParser.T__0);
	                    this.state = 582;
	                    this.match(TzdLangParser.IDENTIFIER);
	                    break;

	                } 
	            }
	            this.state = 587;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,70,this._ctx);
	        }

	    } catch( error) {
	        if(error instanceof antlr4.error.RecognitionException) {
		        localctx.exception = error;
		        this._errHandler.reportError(this, error);
		        this._errHandler.recover(this, error);
		    } else {
		    	throw error;
		    }
	    } finally {
	        this.unrollRecursionContexts(_parentctx)
	    }
	    return localctx;
	}



	classOverrideBlock() {
	    let localctx = new ClassOverrideBlockContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 54, TzdLangParser.RULE_classOverrideBlock);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 588;
	        this.match(TzdLangParser.T__4);
	        this.state = 592;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(((((_la - 8)) & ~0x1f) === 0 && ((1 << (_la - 8)) & 31530753) !== 0) || _la===95) {
	            this.state = 589;
	            this.classMember();
	            this.state = 594;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	        this.state = 595;
	        this.match(TzdLangParser.T__5);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	printFunction() {
	    let localctx = new PrintFunctionContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 56, TzdLangParser.RULE_printFunction);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 597;
	        this.match(TzdLangParser.KW_PRINT);
	        this.state = 598;
	        this.match(TzdLangParser.T__2);
	        this.state = 600;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if((((_la) & ~0x1f) === 0 && ((1 << _la) & 75505704) !== 0) || ((((_la - 32)) & ~0x1f) === 0 && ((1 << (_la - 32)) & 234911745) !== 0) || ((((_la - 81)) & ~0x1f) === 0 && ((1 << (_la - 81)) & 246881) !== 0)) {
	            this.state = 599;
	            this.exprList();
	        }

	        this.state = 602;
	        this.match(TzdLangParser.T__3);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	exprList() {
	    let localctx = new ExprListContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 58, TzdLangParser.RULE_exprList);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 604;
	        this.expression(0);
	        this.state = 609;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 605;
	            this.match(TzdLangParser.T__8);
	            this.state = 606;
	            this.expression(0);
	            this.state = 611;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	mapEntryList() {
	    let localctx = new MapEntryListContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 60, TzdLangParser.RULE_mapEntryList);
	    var _la = 0;
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 612;
	        this.mapEntry();
	        this.state = 617;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,74,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                this.state = 613;
	                this.match(TzdLangParser.T__8);
	                this.state = 614;
	                this.mapEntry(); 
	            }
	            this.state = 619;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,74,this._ctx);
	        }

	        this.state = 621;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===9) {
	            this.state = 620;
	            this.match(TzdLangParser.T__8);
	        }

	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	mapEntry() {
	    let localctx = new MapEntryContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 62, TzdLangParser.RULE_mapEntry);
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 623;
	        this.mapKey();
	        this.state = 624;
	        this.match(TzdLangParser.T__6);
	        this.state = 625;
	        this.expression(0);
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}



	mapKey() {
	    let localctx = new MapKeyContext(this, this._ctx, this.state);
	    this.enterRule(localctx, 64, TzdLangParser.RULE_mapKey);
	    try {
	        this.state = 634;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 98:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 627;
	            this.match(TzdLangParser.STRING);
	            break;
	        case 95:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 628;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;
	        case 96:
	            this.enterOuterAlt(localctx, 3);
	            this.state = 629;
	            this.match(TzdLangParser.INTEGER);
	            break;
	        case 3:
	            this.enterOuterAlt(localctx, 4);
	            this.state = 630;
	            this.match(TzdLangParser.T__2);
	            this.state = 631;
	            this.expression(0);
	            this.state = 632;
	            this.match(TzdLangParser.T__3);
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	    } catch (re) {
	    	if(re instanceof antlr4.error.RecognitionException) {
		        localctx.exception = re;
		        this._errHandler.reportError(this, re);
		        this._errHandler.recover(this, re);
		    } else {
		    	throw re;
		    }
	    } finally {
	        this.exitRule();
	    }
	    return localctx;
	}


	typeType(_p) {
		if(_p===undefined) {
		    _p = 0;
		}
	    const _parentctx = this._ctx;
	    const _parentState = this.state;
	    let localctx = new TypeTypeContext(this, this._ctx, _parentState);
	    let _prevctx = localctx;
	    const _startState = 66;
	    this.enterRecursionRule(localctx, 66, TzdLangParser.RULE_typeType, _p);
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 645;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 50:
	            this.state = 637;
	            this.match(TzdLangParser.T_INT);
	            break;
	        case 51:
	            this.state = 638;
	            this.match(TzdLangParser.T_FLOAT);
	            break;
	        case 52:
	            this.state = 639;
	            this.match(TzdLangParser.T_STRING);
	            break;
	        case 53:
	            this.state = 640;
	            this.match(TzdLangParser.T_BOOL);
	            break;
	        case 54:
	            this.state = 641;
	            this.match(TzdLangParser.T_VOID);
	            break;
	        case 55:
	            this.state = 642;
	            this.match(TzdLangParser.T_PTR);
	            break;
	        case 56:
	            this.state = 643;
	            this.match(TzdLangParser.T_FUNCTION);
	            break;
	        case 95:
	            this.state = 644;
	            this.qualifiedName();
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	        this._ctx.stop = this._input.LT(-1);
	        this.state = 651;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,78,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                if(this._parseListeners!==null) {
	                    this.triggerExitRuleEvent();
	                }
	                _prevctx = localctx;
	                localctx = new TypeTypeContext(this, _parentctx, _parentState);
	                this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_typeType);
	                this.state = 647;
	                if (!( this.precpred(this._ctx, 1))) {
	                    throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 1)");
	                }
	                this.state = 648;
	                this.match(TzdLangParser.T__14); 
	            }
	            this.state = 653;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,78,this._ctx);
	        }

	    } catch( error) {
	        if(error instanceof antlr4.error.RecognitionException) {
		        localctx.exception = error;
		        this._errHandler.reportError(this, error);
		        this._errHandler.recover(this, error);
		    } else {
		    	throw error;
		    }
	    } finally {
	        this.unrollRecursionContexts(_parentctx)
	    }
	    return localctx;
	}


}

TzdLangParser.EOF = antlr4.Token.EOF;
TzdLangParser.T__0 = 1;
TzdLangParser.T__1 = 2;
TzdLangParser.T__2 = 3;
TzdLangParser.T__3 = 4;
TzdLangParser.T__4 = 5;
TzdLangParser.T__5 = 6;
TzdLangParser.T__6 = 7;
TzdLangParser.T__7 = 8;
TzdLangParser.T__8 = 9;
TzdLangParser.T__9 = 10;
TzdLangParser.T__10 = 11;
TzdLangParser.T__11 = 12;
TzdLangParser.T__12 = 13;
TzdLangParser.T__13 = 14;
TzdLangParser.T__14 = 15;
TzdLangParser.KW_VAR = 16;
TzdLangParser.KW_CONST = 17;
TzdLangParser.KW_LET = 18;
TzdLangParser.KW_STATIC = 19;
TzdLangParser.KW_ABSTRACT = 20;
TzdLangParser.KW_ENUM = 21;
TzdLangParser.KW_IN = 22;
TzdLangParser.KW_SUPER = 23;
TzdLangParser.KW_NATIVE = 24;
TzdLangParser.KW_IMPORT = 25;
TzdLangParser.KW_PRINT = 26;
TzdLangParser.KW_CLASS = 27;
TzdLangParser.KW_EXTENDS = 28;
TzdLangParser.KW_PUBLIC = 29;
TzdLangParser.KW_PRIVATE = 30;
TzdLangParser.KW_PROTECTED = 31;
TzdLangParser.KW_FUN = 32;
TzdLangParser.KW_RET = 33;
TzdLangParser.KW_IF = 34;
TzdLangParser.KW_ELSE = 35;
TzdLangParser.KW_WHILE = 36;
TzdLangParser.KW_FOR = 37;
TzdLangParser.KW_BREAK = 38;
TzdLangParser.KW_CONTINUE = 39;
TzdLangParser.KW_SWITCH = 40;
TzdLangParser.KW_CASE = 41;
TzdLangParser.KW_DEFAULT = 42;
TzdLangParser.KW_NEW = 43;
TzdLangParser.KW_TRUE = 44;
TzdLangParser.KW_FALSE = 45;
TzdLangParser.KW_NULL = 46;
TzdLangParser.KW_THROW = 47;
TzdLangParser.KW_TRY = 48;
TzdLangParser.KW_CATCH = 49;
TzdLangParser.T_INT = 50;
TzdLangParser.T_FLOAT = 51;
TzdLangParser.T_STRING = 52;
TzdLangParser.T_BOOL = 53;
TzdLangParser.T_VOID = 54;
TzdLangParser.T_PTR = 55;
TzdLangParser.T_FUNCTION = 56;
TzdLangParser.INC = 57;
TzdLangParser.DEC = 58;
TzdLangParser.GXXX = 59;
TzdLangParser.USHR_ASSIGN = 60;
TzdLangParser.SHR_ASSIGN = 61;
TzdLangParser.SHL_ASSIGN = 62;
TzdLangParser.USHR = 63;
TzdLangParser.SHR = 64;
TzdLangParser.SHL = 65;
TzdLangParser.AND_ASSIGN = 66;
TzdLangParser.OR_ASSIGN = 67;
TzdLangParser.XOR_ASSIGN = 68;
TzdLangParser.MOD_ASSIGN = 69;
TzdLangParser.PLUS_ASSIGN = 70;
TzdLangParser.MIN_ASSIGN = 71;
TzdLangParser.MUL_ASSIGN = 72;
TzdLangParser.DIV_ASSIGN = 73;
TzdLangParser.EEQ = 74;
TzdLangParser.NEQ = 75;
TzdLangParser.GE = 76;
TzdLangParser.LE = 77;
TzdLangParser.AND = 78;
TzdLangParser.OR = 79;
TzdLangParser.POW = 80;
TzdLangParser.BIT_NOT = 81;
TzdLangParser.BIT_AND = 82;
TzdLangParser.BIT_OR = 83;
TzdLangParser.BIT_XOR = 84;
TzdLangParser.ARROW = 85;
TzdLangParser.PLUS = 86;
TzdLangParser.MINUS = 87;
TzdLangParser.MUL = 88;
TzdLangParser.DIV = 89;
TzdLangParser.MOD = 90;
TzdLangParser.NOT = 91;
TzdLangParser.GT = 92;
TzdLangParser.LT = 93;
TzdLangParser.ASSIGN = 94;
TzdLangParser.IDENTIFIER = 95;
TzdLangParser.INTEGER = 96;
TzdLangParser.FLOAT = 97;
TzdLangParser.STRING = 98;
TzdLangParser.LINE_COMMENT = 99;
TzdLangParser.BLOCK_COMMENT = 100;
TzdLangParser.WS = 101;

TzdLangParser.RULE_program = 0;
TzdLangParser.RULE_qualifiedName = 1;
TzdLangParser.RULE_statement = 2;
TzdLangParser.RULE_switchCase = 3;
TzdLangParser.RULE_switchDefault = 4;
TzdLangParser.RULE_annotationDeclaration = 5;
TzdLangParser.RULE_enumDeclaration = 6;
TzdLangParser.RULE_enumList = 7;
TzdLangParser.RULE_classDeclaration = 8;
TzdLangParser.RULE_classBody = 9;
TzdLangParser.RULE_classMember = 10;
TzdLangParser.RULE_memberDecl = 11;
TzdLangParser.RULE_annotationUsage = 12;
TzdLangParser.RULE_accessModifier = 13;
TzdLangParser.RULE_functionDeclaration = 14;
TzdLangParser.RULE_nativeFunctionDeclaration = 15;
TzdLangParser.RULE_nativeAttrList = 16;
TzdLangParser.RULE_nativeAttr = 17;
TzdLangParser.RULE_nativePropKey = 18;
TzdLangParser.RULE_variableDeclaration = 19;
TzdLangParser.RULE_forInit = 20;
TzdLangParser.RULE_importStatement = 21;
TzdLangParser.RULE_paramList = 22;
TzdLangParser.RULE_param = 23;
TzdLangParser.RULE_block = 24;
TzdLangParser.RULE_expression = 25;
TzdLangParser.RULE_atom = 26;
TzdLangParser.RULE_classOverrideBlock = 27;
TzdLangParser.RULE_printFunction = 28;
TzdLangParser.RULE_exprList = 29;
TzdLangParser.RULE_mapEntryList = 30;
TzdLangParser.RULE_mapEntry = 31;
TzdLangParser.RULE_mapKey = 32;
TzdLangParser.RULE_typeType = 33;

class ProgramContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_program;
    }

	EOF() {
	    return this.getToken(TzdLangParser.EOF, 0);
	};

	statement = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(StatementContext);
	    } else {
	        return this.getTypedRuleContext(StatementContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterProgram(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitProgram(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitProgram(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class QualifiedNameContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_qualifiedName;
    }

	IDENTIFIER = function(i) {
		if(i===undefined) {
			i = null;
		}
	    if(i===null) {
	        return this.getTokens(TzdLangParser.IDENTIFIER);
	    } else {
	        return this.getToken(TzdLangParser.IDENTIFIER, i);
	    }
	};


	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterQualifiedName(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitQualifiedName(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitQualifiedName(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class StatementContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_statement;
    }


	 
		copyFrom(ctx) {
			super.copyFrom(ctx);
		}

}


class SwitchStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_SWITCH() {
	    return this.getToken(TzdLangParser.KW_SWITCH, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	switchCase = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(SwitchCaseContext);
	    } else {
	        return this.getTypedRuleContext(SwitchCaseContext,i);
	    }
	};

	switchDefault() {
	    return this.getTypedRuleContext(SwitchDefaultContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterSwitchStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitSwitchStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitSwitchStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.SwitchStmtContext = SwitchStmtContext;

class ClassDeclStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	classDeclaration() {
	    return this.getTypedRuleContext(ClassDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterClassDeclStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitClassDeclStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitClassDeclStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ClassDeclStmtContext = ClassDeclStmtContext;

class BlockStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	block() {
	    return this.getTypedRuleContext(BlockContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBlockStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBlockStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBlockStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BlockStmtContext = BlockStmtContext;

class NativeFunDeclStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	nativeFunctionDeclaration() {
	    return this.getTypedRuleContext(NativeFunctionDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNativeFunDeclStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNativeFunDeclStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNativeFunDeclStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.NativeFunDeclStmtContext = NativeFunDeclStmtContext;

class ContinueStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_CONTINUE() {
	    return this.getToken(TzdLangParser.KW_CONTINUE, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterContinueStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitContinueStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitContinueStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ContinueStmtContext = ContinueStmtContext;

class ImportStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	importStatement() {
	    return this.getTypedRuleContext(ImportStatementContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterImportStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitImportStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitImportStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ImportStmtContext = ImportStmtContext;

class IfStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_IF() {
	    return this.getToken(TzdLangParser.KW_IF, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	statement = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(StatementContext);
	    } else {
	        return this.getTypedRuleContext(StatementContext,i);
	    }
	};

	KW_ELSE() {
	    return this.getToken(TzdLangParser.KW_ELSE, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterIfStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitIfStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitIfStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.IfStmtContext = IfStmtContext;

class ExprStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterExprStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitExprStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitExprStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ExprStmtContext = ExprStmtContext;

class WhileStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_WHILE() {
	    return this.getToken(TzdLangParser.KW_WHILE, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	statement() {
	    return this.getTypedRuleContext(StatementContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterWhileStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitWhileStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitWhileStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.WhileStmtContext = WhileStmtContext;

class AnnotationDeclStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	annotationDeclaration() {
	    return this.getTypedRuleContext(AnnotationDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAnnotationDeclStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAnnotationDeclStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAnnotationDeclStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.AnnotationDeclStmtContext = AnnotationDeclStmtContext;

class VarDeclStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	variableDeclaration() {
	    return this.getTypedRuleContext(VariableDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterVarDeclStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitVarDeclStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitVarDeclStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.VarDeclStmtContext = VarDeclStmtContext;

class BreakStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_BREAK() {
	    return this.getToken(TzdLangParser.KW_BREAK, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBreakStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBreakStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBreakStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BreakStmtContext = BreakStmtContext;

class EnumDeclStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	enumDeclaration() {
	    return this.getTypedRuleContext(EnumDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterEnumDeclStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitEnumDeclStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitEnumDeclStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.EnumDeclStmtContext = EnumDeclStmtContext;

class EmptyStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }


	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterEmptyStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitEmptyStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitEmptyStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.EmptyStmtContext = EmptyStmtContext;

class ReturnStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_RET() {
	    return this.getToken(TzdLangParser.KW_RET, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterReturnStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitReturnStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitReturnStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ReturnStmtContext = ReturnStmtContext;

class ForStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        this.cond = null;;
        this.step = null;;
        super.copyFrom(ctx);
    }

	KW_FOR() {
	    return this.getToken(TzdLangParser.KW_FOR, 0);
	};

	statement() {
	    return this.getTypedRuleContext(StatementContext,0);
	};

	forInit() {
	    return this.getTypedRuleContext(ForInitContext,0);
	};

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterForStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitForStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitForStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ForStmtContext = ForStmtContext;

class ThrowStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_THROW() {
	    return this.getToken(TzdLangParser.KW_THROW, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterThrowStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitThrowStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitThrowStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ThrowStmtContext = ThrowStmtContext;

class FunDeclStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	functionDeclaration() {
	    return this.getTypedRuleContext(FunctionDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterFunDeclStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitFunDeclStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitFunDeclStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.FunDeclStmtContext = FunDeclStmtContext;

class TryCatchStmtContext extends StatementContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_TRY() {
	    return this.getToken(TzdLangParser.KW_TRY, 0);
	};

	block = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(BlockContext);
	    } else {
	        return this.getTypedRuleContext(BlockContext,i);
	    }
	};

	KW_CATCH() {
	    return this.getToken(TzdLangParser.KW_CATCH, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterTryCatchStmt(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitTryCatchStmt(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitTryCatchStmt(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.TryCatchStmtContext = TryCatchStmtContext;

class SwitchCaseContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_switchCase;
    }

	KW_CASE() {
	    return this.getToken(TzdLangParser.KW_CASE, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	statement = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(StatementContext);
	    } else {
	        return this.getTypedRuleContext(StatementContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterSwitchCase(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitSwitchCase(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitSwitchCase(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class SwitchDefaultContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_switchDefault;
    }

	KW_DEFAULT() {
	    return this.getToken(TzdLangParser.KW_DEFAULT, 0);
	};

	statement = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(StatementContext);
	    } else {
	        return this.getTypedRuleContext(StatementContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterSwitchDefault(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitSwitchDefault(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitSwitchDefault(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class AnnotationDeclarationContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_annotationDeclaration;
    }

	KW_CLASS() {
	    return this.getToken(TzdLangParser.KW_CLASS, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAnnotationDeclaration(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAnnotationDeclaration(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAnnotationDeclaration(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class EnumDeclarationContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_enumDeclaration;
    }

	KW_ENUM() {
	    return this.getToken(TzdLangParser.KW_ENUM, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	enumList() {
	    return this.getTypedRuleContext(EnumListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterEnumDeclaration(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitEnumDeclaration(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitEnumDeclaration(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class EnumListContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_enumList;
    }

	IDENTIFIER = function(i) {
		if(i===undefined) {
			i = null;
		}
	    if(i===null) {
	        return this.getTokens(TzdLangParser.IDENTIFIER);
	    } else {
	        return this.getToken(TzdLangParser.IDENTIFIER, i);
	    }
	};


	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterEnumList(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitEnumList(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitEnumList(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ClassDeclarationContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_classDeclaration;
    }

	KW_CLASS() {
	    return this.getToken(TzdLangParser.KW_CLASS, 0);
	};

	qualifiedName = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(QualifiedNameContext);
	    } else {
	        return this.getTypedRuleContext(QualifiedNameContext,i);
	    }
	};

	classBody() {
	    return this.getTypedRuleContext(ClassBodyContext,0);
	};

	annotationUsage() {
	    return this.getTypedRuleContext(AnnotationUsageContext,0);
	};

	KW_EXTENDS() {
	    return this.getToken(TzdLangParser.KW_EXTENDS, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterClassDeclaration(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitClassDeclaration(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitClassDeclaration(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ClassBodyContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_classBody;
    }

	classMember = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ClassMemberContext);
	    } else {
	        return this.getTypedRuleContext(ClassMemberContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterClassBody(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitClassBody(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitClassBody(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ClassMemberContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_classMember;
    }

	memberDecl() {
	    return this.getTypedRuleContext(MemberDeclContext,0);
	};

	annotationUsage() {
	    return this.getTypedRuleContext(AnnotationUsageContext,0);
	};

	accessModifier() {
	    return this.getTypedRuleContext(AccessModifierContext,0);
	};

	nativeFunctionDeclaration() {
	    return this.getTypedRuleContext(NativeFunctionDeclarationContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterClassMember(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitClassMember(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitClassMember(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class MemberDeclContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_memberDecl;
    }


	 
		copyFrom(ctx) {
			super.copyFrom(ctx);
		}

}


class ConstructorDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	block() {
	    return this.getTypedRuleContext(BlockContext,0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterConstructorDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitConstructorDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitConstructorDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ConstructorDeclContext = ConstructorDeclContext;

class FieldVarDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_VAR() {
	    return this.getToken(TzdLangParser.KW_VAR, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	ASSIGN() {
	    return this.getToken(TzdLangParser.ASSIGN, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterFieldVarDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitFieldVarDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitFieldVarDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.FieldVarDeclContext = FieldVarDeclContext;

class MethodStaticDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_STATIC() {
	    return this.getToken(TzdLangParser.KW_STATIC, 0);
	};

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	block() {
	    return this.getTypedRuleContext(BlockContext,0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	ARROW() {
	    return this.getToken(TzdLangParser.ARROW, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMethodStaticDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMethodStaticDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMethodStaticDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.MethodStaticDeclContext = MethodStaticDeclContext;

class MethodDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	block() {
	    return this.getTypedRuleContext(BlockContext,0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	ARROW() {
	    return this.getToken(TzdLangParser.ARROW, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMethodDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMethodDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMethodDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.MethodDeclContext = MethodDeclContext;

class MethodAbstractDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_ABSTRACT() {
	    return this.getToken(TzdLangParser.KW_ABSTRACT, 0);
	};

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	ARROW() {
	    return this.getToken(TzdLangParser.ARROW, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMethodAbstractDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMethodAbstractDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMethodAbstractDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.MethodAbstractDeclContext = MethodAbstractDeclContext;

class FieldConstDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_CONST() {
	    return this.getToken(TzdLangParser.KW_CONST, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	ASSIGN() {
	    return this.getToken(TzdLangParser.ASSIGN, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterFieldConstDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitFieldConstDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitFieldConstDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.FieldConstDeclContext = FieldConstDeclContext;

class FieldLetDeclContext extends MemberDeclContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_LET() {
	    return this.getToken(TzdLangParser.KW_LET, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	ASSIGN() {
	    return this.getToken(TzdLangParser.ASSIGN, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterFieldLetDecl(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitFieldLetDecl(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitFieldLetDecl(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.FieldLetDeclContext = FieldLetDeclContext;

class AnnotationUsageContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_annotationUsage;
    }

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	exprList() {
	    return this.getTypedRuleContext(ExprListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAnnotationUsage(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAnnotationUsage(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAnnotationUsage(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class AccessModifierContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_accessModifier;
    }

	KW_PUBLIC() {
	    return this.getToken(TzdLangParser.KW_PUBLIC, 0);
	};

	KW_PRIVATE() {
	    return this.getToken(TzdLangParser.KW_PRIVATE, 0);
	};

	KW_PROTECTED() {
	    return this.getToken(TzdLangParser.KW_PROTECTED, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAccessModifier(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAccessModifier(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAccessModifier(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class FunctionDeclarationContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_functionDeclaration;
    }

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	block() {
	    return this.getTypedRuleContext(BlockContext,0);
	};

	annotationUsage() {
	    return this.getTypedRuleContext(AnnotationUsageContext,0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	ARROW() {
	    return this.getToken(TzdLangParser.ARROW, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterFunctionDeclaration(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitFunctionDeclaration(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitFunctionDeclaration(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class NativeFunctionDeclarationContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_nativeFunctionDeclaration;
    }

	KW_NATIVE() {
	    return this.getToken(TzdLangParser.KW_NATIVE, 0);
	};

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	annotationUsage() {
	    return this.getTypedRuleContext(AnnotationUsageContext,0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	ARROW() {
	    return this.getToken(TzdLangParser.ARROW, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	nativeAttrList() {
	    return this.getTypedRuleContext(NativeAttrListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNativeFunctionDeclaration(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNativeFunctionDeclaration(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNativeFunctionDeclaration(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class NativeAttrListContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_nativeAttrList;
    }

	nativeAttr = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(NativeAttrContext);
	    } else {
	        return this.getTypedRuleContext(NativeAttrContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNativeAttrList(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNativeAttrList(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNativeAttrList(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class NativeAttrContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_nativeAttr;
    }

	ASSIGN() {
	    return this.getToken(TzdLangParser.ASSIGN, 0);
	};

	STRING() {
	    return this.getToken(TzdLangParser.STRING, 0);
	};

	INTEGER() {
	    return this.getToken(TzdLangParser.INTEGER, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	nativePropKey() {
	    return this.getTypedRuleContext(NativePropKeyContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNativeAttr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNativeAttr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNativeAttr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class NativePropKeyContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_nativePropKey;
    }

	T_INT() {
	    return this.getToken(TzdLangParser.T_INT, 0);
	};

	T_STRING() {
	    return this.getToken(TzdLangParser.T_STRING, 0);
	};

	T_FLOAT() {
	    return this.getToken(TzdLangParser.T_FLOAT, 0);
	};

	T_BOOL() {
	    return this.getToken(TzdLangParser.T_BOOL, 0);
	};

	T_VOID() {
	    return this.getToken(TzdLangParser.T_VOID, 0);
	};

	T_PTR() {
	    return this.getToken(TzdLangParser.T_PTR, 0);
	};

	T_FUNCTION() {
	    return this.getToken(TzdLangParser.T_FUNCTION, 0);
	};

	KW_RET() {
	    return this.getToken(TzdLangParser.KW_RET, 0);
	};

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNativePropKey(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNativePropKey(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNativePropKey(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class VariableDeclarationContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_variableDeclaration;
    }

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	ASSIGN() {
	    return this.getToken(TzdLangParser.ASSIGN, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	KW_VAR() {
	    return this.getToken(TzdLangParser.KW_VAR, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterVariableDeclaration(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitVariableDeclaration(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitVariableDeclaration(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ForInitContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_forInit;
    }

	variableDeclaration() {
	    return this.getTypedRuleContext(VariableDeclarationContext,0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterForInit(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitForInit(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitForInit(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ImportStatementContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_importStatement;
    }

	KW_IMPORT() {
	    return this.getToken(TzdLangParser.KW_IMPORT, 0);
	};

	STRING() {
	    return this.getToken(TzdLangParser.STRING, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterImportStatement(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitImportStatement(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitImportStatement(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ParamListContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_paramList;
    }

	param = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ParamContext);
	    } else {
	        return this.getTypedRuleContext(ParamContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterParamList(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitParamList(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitParamList(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ParamContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_param;
    }

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	T_INT() {
	    return this.getToken(TzdLangParser.T_INT, 0);
	};

	T_STRING() {
	    return this.getToken(TzdLangParser.T_STRING, 0);
	};

	T_FLOAT() {
	    return this.getToken(TzdLangParser.T_FLOAT, 0);
	};

	T_BOOL() {
	    return this.getToken(TzdLangParser.T_BOOL, 0);
	};

	T_VOID() {
	    return this.getToken(TzdLangParser.T_VOID, 0);
	};

	T_PTR() {
	    return this.getToken(TzdLangParser.T_PTR, 0);
	};

	T_FUNCTION() {
	    return this.getToken(TzdLangParser.T_FUNCTION, 0);
	};

	KW_RET() {
	    return this.getToken(TzdLangParser.KW_RET, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterParam(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitParam(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitParam(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class BlockContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_block;
    }

	statement = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(StatementContext);
	    } else {
	        return this.getTypedRuleContext(StatementContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBlock(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBlock(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBlock(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ExpressionContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_expression;
    }


	 
		copyFrom(ctx) {
			super.copyFrom(ctx);
		}

}


class TypeCheckExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	KW_IN() {
	    return this.getToken(TzdLangParser.KW_IN, 0);
	};

	qualifiedName() {
	    return this.getTypedRuleContext(QualifiedNameContext,0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterTypeCheckExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitTypeCheckExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitTypeCheckExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.TypeCheckExprContext = TypeCheckExprContext;

class BitAndExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	BIT_AND() {
	    return this.getToken(TzdLangParser.BIT_AND, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBitAndExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBitAndExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBitAndExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BitAndExprContext = BitAndExprContext;

class RelationalExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	GT() {
	    return this.getToken(TzdLangParser.GT, 0);
	};

	LT() {
	    return this.getToken(TzdLangParser.LT, 0);
	};

	GE() {
	    return this.getToken(TzdLangParser.GE, 0);
	};

	LE() {
	    return this.getToken(TzdLangParser.LE, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterRelationalExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitRelationalExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitRelationalExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.RelationalExprContext = RelationalExprContext;

class AssignmentExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	ASSIGN() {
	    return this.getToken(TzdLangParser.ASSIGN, 0);
	};

	PLUS_ASSIGN() {
	    return this.getToken(TzdLangParser.PLUS_ASSIGN, 0);
	};

	MIN_ASSIGN() {
	    return this.getToken(TzdLangParser.MIN_ASSIGN, 0);
	};

	MUL_ASSIGN() {
	    return this.getToken(TzdLangParser.MUL_ASSIGN, 0);
	};

	DIV_ASSIGN() {
	    return this.getToken(TzdLangParser.DIV_ASSIGN, 0);
	};

	MOD_ASSIGN() {
	    return this.getToken(TzdLangParser.MOD_ASSIGN, 0);
	};

	AND_ASSIGN() {
	    return this.getToken(TzdLangParser.AND_ASSIGN, 0);
	};

	OR_ASSIGN() {
	    return this.getToken(TzdLangParser.OR_ASSIGN, 0);
	};

	XOR_ASSIGN() {
	    return this.getToken(TzdLangParser.XOR_ASSIGN, 0);
	};

	SHL_ASSIGN() {
	    return this.getToken(TzdLangParser.SHL_ASSIGN, 0);
	};

	SHR_ASSIGN() {
	    return this.getToken(TzdLangParser.SHR_ASSIGN, 0);
	};

	USHR_ASSIGN() {
	    return this.getToken(TzdLangParser.USHR_ASSIGN, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAssignmentExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAssignmentExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAssignmentExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.AssignmentExprContext = AssignmentExprContext;

class AtomExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	atom() {
	    return this.getTypedRuleContext(AtomContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAtomExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAtomExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAtomExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.AtomExprContext = AtomExprContext;

class BitOrExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	BIT_OR() {
	    return this.getToken(TzdLangParser.BIT_OR, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBitOrExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBitOrExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBitOrExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BitOrExprContext = BitOrExprContext;

class UnaryExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	PLUS() {
	    return this.getToken(TzdLangParser.PLUS, 0);
	};

	MINUS() {
	    return this.getToken(TzdLangParser.MINUS, 0);
	};

	NOT() {
	    return this.getToken(TzdLangParser.NOT, 0);
	};

	BIT_NOT() {
	    return this.getToken(TzdLangParser.BIT_NOT, 0);
	};

	GXXX() {
	    return this.getToken(TzdLangParser.GXXX, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterUnaryExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitUnaryExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitUnaryExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.UnaryExprContext = UnaryExprContext;

class LogicalAndExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	AND() {
	    return this.getToken(TzdLangParser.AND, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterLogicalAndExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitLogicalAndExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitLogicalAndExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.LogicalAndExprContext = LogicalAndExprContext;

class IndexExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterIndexExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitIndexExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitIndexExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.IndexExprContext = IndexExprContext;

class PrefixExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	INC() {
	    return this.getToken(TzdLangParser.INC, 0);
	};

	DEC() {
	    return this.getToken(TzdLangParser.DEC, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterPrefixExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitPrefixExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitPrefixExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.PrefixExprContext = PrefixExprContext;

class PostfixExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	INC() {
	    return this.getToken(TzdLangParser.INC, 0);
	};

	DEC() {
	    return this.getToken(TzdLangParser.DEC, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterPostfixExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitPostfixExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitPostfixExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.PostfixExprContext = PostfixExprContext;

class PowerExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	POW() {
	    return this.getToken(TzdLangParser.POW, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterPowerExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitPowerExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitPowerExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.PowerExprContext = PowerExprContext;

class MultiplicativeExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	MUL() {
	    return this.getToken(TzdLangParser.MUL, 0);
	};

	DIV() {
	    return this.getToken(TzdLangParser.DIV, 0);
	};

	MOD() {
	    return this.getToken(TzdLangParser.MOD, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMultiplicativeExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMultiplicativeExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMultiplicativeExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.MultiplicativeExprContext = MultiplicativeExprContext;

class LogicalOrExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	OR() {
	    return this.getToken(TzdLangParser.OR, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterLogicalOrExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitLogicalOrExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitLogicalOrExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.LogicalOrExprContext = LogicalOrExprContext;

class EqualityExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	EEQ() {
	    return this.getToken(TzdLangParser.EEQ, 0);
	};

	NEQ() {
	    return this.getToken(TzdLangParser.NEQ, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterEqualityExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitEqualityExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitEqualityExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.EqualityExprContext = EqualityExprContext;

class AdditiveExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	PLUS() {
	    return this.getToken(TzdLangParser.PLUS, 0);
	};

	MINUS() {
	    return this.getToken(TzdLangParser.MINUS, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterAdditiveExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitAdditiveExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitAdditiveExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.AdditiveExprContext = AdditiveExprContext;

class CastExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterCastExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitCastExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitCastExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.CastExprContext = CastExprContext;

class ShiftExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	SHL() {
	    return this.getToken(TzdLangParser.SHL, 0);
	};

	SHR() {
	    return this.getToken(TzdLangParser.SHR, 0);
	};

	USHR() {
	    return this.getToken(TzdLangParser.USHR, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterShiftExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitShiftExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitShiftExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ShiftExprContext = ShiftExprContext;

class BitXorExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	BIT_XOR() {
	    return this.getToken(TzdLangParser.BIT_XOR, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBitXorExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBitXorExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBitXorExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BitXorExprContext = BitXorExprContext;

class AtomContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_atom;
    }


	 
		copyFrom(ctx) {
			super.copyFrom(ctx);
		}

}


class BoolTrueExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_TRUE() {
	    return this.getToken(TzdLangParser.KW_TRUE, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBoolTrueExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBoolTrueExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBoolTrueExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BoolTrueExprContext = BoolTrueExprContext;

class StringExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	STRING() {
	    return this.getToken(TzdLangParser.STRING, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterStringExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitStringExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitStringExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.StringExprContext = StringExprContext;

class FloatExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	FLOAT() {
	    return this.getToken(TzdLangParser.FLOAT, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterFloatExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitFloatExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitFloatExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.FloatExprContext = FloatExprContext;

class BoolFalseExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_FALSE() {
	    return this.getToken(TzdLangParser.KW_FALSE, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterBoolFalseExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitBoolFalseExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitBoolFalseExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.BoolFalseExprContext = BoolFalseExprContext;

class IdExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterIdExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitIdExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitIdExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.IdExprContext = IdExprContext;

class SuperExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_SUPER() {
	    return this.getToken(TzdLangParser.KW_SUPER, 0);
	};

	exprList() {
	    return this.getTypedRuleContext(ExprListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterSuperExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitSuperExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitSuperExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.SuperExprContext = SuperExprContext;

class LambdaExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_FUN() {
	    return this.getToken(TzdLangParser.KW_FUN, 0);
	};

	block() {
	    return this.getTypedRuleContext(BlockContext,0);
	};

	paramList() {
	    return this.getTypedRuleContext(ParamListContext,0);
	};

	ARROW() {
	    return this.getToken(TzdLangParser.ARROW, 0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterLambdaExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitLambdaExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitLambdaExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.LambdaExprContext = LambdaExprContext;

class NullExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_NULL() {
	    return this.getToken(TzdLangParser.KW_NULL, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNullExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNullExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNullExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.NullExprContext = NullExprContext;

class PrintFunExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	printFunction() {
	    return this.getTypedRuleContext(PrintFunctionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterPrintFunExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitPrintFunExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitPrintFunExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.PrintFunExprContext = PrintFunExprContext;

class ArrayLiteralExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	exprList() {
	    return this.getTypedRuleContext(ExprListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterArrayLiteralExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitArrayLiteralExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitArrayLiteralExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ArrayLiteralExprContext = ArrayLiteralExprContext;

class MapLiteralExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	mapEntryList() {
	    return this.getTypedRuleContext(MapEntryListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMapLiteralExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMapLiteralExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMapLiteralExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.MapLiteralExprContext = MapLiteralExprContext;

class NewExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	KW_NEW() {
	    return this.getToken(TzdLangParser.KW_NEW, 0);
	};

	qualifiedName() {
	    return this.getTypedRuleContext(QualifiedNameContext,0);
	};

	exprList() {
	    return this.getTypedRuleContext(ExprListContext,0);
	};

	classOverrideBlock() {
	    return this.getTypedRuleContext(ClassOverrideBlockContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterNewExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitNewExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitNewExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.NewExprContext = NewExprContext;

class CallExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	atom() {
	    return this.getTypedRuleContext(AtomContext,0);
	};

	exprList() {
	    return this.getTypedRuleContext(ExprListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterCallExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitCallExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitCallExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.CallExprContext = CallExprContext;

class IntExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	INTEGER() {
	    return this.getToken(TzdLangParser.INTEGER, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterIntExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitIntExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitIntExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.IntExprContext = IntExprContext;

class ParenExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterParenExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitParenExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitParenExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.ParenExprContext = ParenExprContext;

class MemberAccessExprContext extends AtomContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	atom() {
	    return this.getTypedRuleContext(AtomContext,0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMemberAccessExpr(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMemberAccessExpr(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMemberAccessExpr(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}

TzdLangParser.MemberAccessExprContext = MemberAccessExprContext;

class ClassOverrideBlockContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_classOverrideBlock;
    }

	classMember = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ClassMemberContext);
	    } else {
	        return this.getTypedRuleContext(ClassMemberContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterClassOverrideBlock(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitClassOverrideBlock(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitClassOverrideBlock(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class PrintFunctionContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_printFunction;
    }

	KW_PRINT() {
	    return this.getToken(TzdLangParser.KW_PRINT, 0);
	};

	exprList() {
	    return this.getTypedRuleContext(ExprListContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterPrintFunction(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitPrintFunction(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitPrintFunction(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class ExprListContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_exprList;
    }

	expression = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(ExpressionContext);
	    } else {
	        return this.getTypedRuleContext(ExpressionContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterExprList(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitExprList(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitExprList(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class MapEntryListContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_mapEntryList;
    }

	mapEntry = function(i) {
	    if(i===undefined) {
	        i = null;
	    }
	    if(i===null) {
	        return this.getTypedRuleContexts(MapEntryContext);
	    } else {
	        return this.getTypedRuleContext(MapEntryContext,i);
	    }
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMapEntryList(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMapEntryList(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMapEntryList(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class MapEntryContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_mapEntry;
    }

	mapKey() {
	    return this.getTypedRuleContext(MapKeyContext,0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMapEntry(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMapEntry(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMapEntry(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class MapKeyContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_mapKey;
    }

	STRING() {
	    return this.getToken(TzdLangParser.STRING, 0);
	};

	IDENTIFIER() {
	    return this.getToken(TzdLangParser.IDENTIFIER, 0);
	};

	INTEGER() {
	    return this.getToken(TzdLangParser.INTEGER, 0);
	};

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterMapKey(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitMapKey(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitMapKey(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}



class TypeTypeContext extends antlr4.ParserRuleContext {

    constructor(parser, parent, invokingState) {
        if(parent===undefined) {
            parent = null;
        }
        if(invokingState===undefined || invokingState===null) {
            invokingState = -1;
        }
        super(parent, invokingState);
        this.parser = parser;
        this.ruleIndex = TzdLangParser.RULE_typeType;
    }

	T_INT() {
	    return this.getToken(TzdLangParser.T_INT, 0);
	};

	T_FLOAT() {
	    return this.getToken(TzdLangParser.T_FLOAT, 0);
	};

	T_STRING() {
	    return this.getToken(TzdLangParser.T_STRING, 0);
	};

	T_BOOL() {
	    return this.getToken(TzdLangParser.T_BOOL, 0);
	};

	T_VOID() {
	    return this.getToken(TzdLangParser.T_VOID, 0);
	};

	T_PTR() {
	    return this.getToken(TzdLangParser.T_PTR, 0);
	};

	T_FUNCTION() {
	    return this.getToken(TzdLangParser.T_FUNCTION, 0);
	};

	qualifiedName() {
	    return this.getTypedRuleContext(QualifiedNameContext,0);
	};

	typeType() {
	    return this.getTypedRuleContext(TypeTypeContext,0);
	};

	enterRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.enterTypeType(this);
		}
	}

	exitRule(listener) {
	    if(listener instanceof TzdLangListener ) {
	        listener.exitTypeType(this);
		}
	}

	accept(visitor) {
	    if ( visitor instanceof TzdLangVisitor ) {
	        return visitor.visitTypeType(this);
	    } else {
	        return visitor.visitChildren(this);
	    }
	}


}




TzdLangParser.ProgramContext = ProgramContext; 
TzdLangParser.QualifiedNameContext = QualifiedNameContext; 
TzdLangParser.StatementContext = StatementContext; 
TzdLangParser.SwitchCaseContext = SwitchCaseContext; 
TzdLangParser.SwitchDefaultContext = SwitchDefaultContext; 
TzdLangParser.AnnotationDeclarationContext = AnnotationDeclarationContext; 
TzdLangParser.EnumDeclarationContext = EnumDeclarationContext; 
TzdLangParser.EnumListContext = EnumListContext; 
TzdLangParser.ClassDeclarationContext = ClassDeclarationContext; 
TzdLangParser.ClassBodyContext = ClassBodyContext; 
TzdLangParser.ClassMemberContext = ClassMemberContext; 
TzdLangParser.MemberDeclContext = MemberDeclContext; 
TzdLangParser.AnnotationUsageContext = AnnotationUsageContext; 
TzdLangParser.AccessModifierContext = AccessModifierContext; 
TzdLangParser.FunctionDeclarationContext = FunctionDeclarationContext; 
TzdLangParser.NativeFunctionDeclarationContext = NativeFunctionDeclarationContext; 
TzdLangParser.NativeAttrListContext = NativeAttrListContext; 
TzdLangParser.NativeAttrContext = NativeAttrContext; 
TzdLangParser.NativePropKeyContext = NativePropKeyContext; 
TzdLangParser.VariableDeclarationContext = VariableDeclarationContext; 
TzdLangParser.ForInitContext = ForInitContext; 
TzdLangParser.ImportStatementContext = ImportStatementContext; 
TzdLangParser.ParamListContext = ParamListContext; 
TzdLangParser.ParamContext = ParamContext; 
TzdLangParser.BlockContext = BlockContext; 
TzdLangParser.ExpressionContext = ExpressionContext; 
TzdLangParser.AtomContext = AtomContext; 
TzdLangParser.ClassOverrideBlockContext = ClassOverrideBlockContext; 
TzdLangParser.PrintFunctionContext = PrintFunctionContext; 
TzdLangParser.ExprListContext = ExprListContext; 
TzdLangParser.MapEntryListContext = MapEntryListContext; 
TzdLangParser.MapEntryContext = MapEntryContext; 
TzdLangParser.MapKeyContext = MapKeyContext; 
TzdLangParser.TypeTypeContext = TypeTypeContext; 
