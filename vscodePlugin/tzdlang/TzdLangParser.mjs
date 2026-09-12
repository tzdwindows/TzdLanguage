// Generated from C:/Users/tzdwindows 7/source/repos/TzdTools/Grammar/TzdLang.g4 by ANTLR 4.13.1
// jshint ignore: start
import antlr4 from 'antlr4';
import TzdLangListener from './TzdLangListener.mjs';
import TzdLangVisitor from './TzdLangVisitor.mjs';

const serializedATN = [4,1,86,584,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,
4,2,5,7,5,2,6,7,6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,
2,13,7,13,2,14,7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,
20,7,20,2,21,7,21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,
7,27,2,28,7,28,2,29,7,29,2,30,7,30,1,0,5,0,64,8,0,10,0,12,0,67,9,0,1,0,1,
0,1,1,1,1,1,1,5,1,74,8,1,10,1,12,1,77,9,1,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
2,1,2,1,2,1,2,1,2,3,2,91,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,101,8,2,
1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,112,8,2,1,2,1,2,3,2,116,8,2,1,2,
1,2,3,2,120,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,5,2,134,
8,2,10,2,12,2,137,9,2,1,2,3,2,140,8,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,
2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,1,2,3,2,160,8,2,1,3,1,3,1,3,1,3,5,3,166,
8,3,10,3,12,3,169,9,3,1,4,1,4,1,4,5,4,174,8,4,10,4,12,4,177,9,4,1,5,1,5,
1,5,1,5,1,5,3,5,184,8,5,1,5,1,5,1,6,1,6,1,6,1,6,3,6,192,8,6,1,6,1,6,1,7,
1,7,1,7,5,7,199,8,7,10,7,12,7,202,9,7,1,8,3,8,205,8,8,1,8,1,8,1,8,1,8,3,
8,211,8,8,1,8,1,8,1,8,1,8,1,8,3,8,218,8,8,1,8,1,8,1,8,1,8,3,8,224,8,8,1,
8,1,8,1,8,1,8,3,8,230,8,8,1,9,5,9,233,8,9,10,9,12,9,236,9,9,1,10,3,10,239,
8,10,1,10,3,10,242,8,10,1,10,1,10,3,10,246,8,10,1,11,1,11,1,11,1,11,1,11,
3,11,253,8,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,262,8,11,1,11,1,11,
1,11,1,11,1,11,1,11,3,11,270,8,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,
1,11,3,11,281,8,11,1,11,1,11,1,11,1,11,1,11,1,11,1,11,3,11,290,8,11,1,11,
1,11,1,11,1,11,1,11,1,11,3,11,298,8,11,1,11,1,11,1,11,1,11,1,11,3,11,305,
8,11,1,11,1,11,3,11,309,8,11,1,12,1,12,1,12,1,12,3,12,315,8,12,1,12,3,12,
318,8,12,1,13,1,13,1,14,3,14,323,8,14,1,14,1,14,1,14,1,14,3,14,329,8,14,
1,14,1,14,1,14,1,15,3,15,335,8,15,1,15,1,15,1,15,1,15,1,15,3,15,342,8,15,
1,15,1,15,1,15,3,15,347,8,15,1,15,1,15,1,15,1,16,1,16,1,16,5,16,355,8,16,
10,16,12,16,358,9,16,1,17,1,17,3,17,362,8,17,1,17,1,17,1,17,1,18,1,18,1,
19,1,19,1,19,1,19,3,19,373,8,19,1,19,1,19,1,19,1,19,3,19,379,8,19,1,19,1,
19,1,19,1,19,1,19,3,19,386,8,19,3,19,388,8,19,1,20,1,20,3,20,392,8,20,1,
21,1,21,1,21,1,21,1,22,1,22,1,22,5,22,401,8,22,10,22,12,22,404,9,22,1,23,
1,23,1,23,1,23,1,23,1,23,3,23,412,8,23,3,23,414,8,23,1,24,1,24,5,24,418,
8,24,10,24,12,24,421,9,24,1,24,1,24,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
25,1,25,1,25,1,25,3,25,436,8,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,3,25,473,
8,25,5,25,475,8,25,10,25,12,25,478,9,25,1,26,1,26,1,26,1,26,1,26,1,26,1,
26,3,26,487,8,26,1,26,1,26,1,26,3,26,492,8,26,1,26,1,26,1,26,1,26,1,26,1,
26,1,26,1,26,1,26,1,26,1,26,3,26,505,8,26,1,26,1,26,1,26,1,26,1,26,1,26,
3,26,513,8,26,1,26,1,26,3,26,517,8,26,1,26,1,26,1,26,3,26,522,8,26,1,26,
1,26,3,26,526,8,26,1,26,1,26,1,26,3,26,531,8,26,1,26,1,26,1,26,1,26,5,26,
537,8,26,10,26,12,26,540,9,26,1,27,1,27,5,27,544,8,27,10,27,12,27,547,9,
27,1,27,1,27,1,28,1,28,1,28,3,28,554,8,28,1,28,1,28,1,29,1,29,1,29,5,29,
561,8,29,10,29,12,29,564,9,29,1,30,1,30,1,30,1,30,1,30,1,30,1,30,1,30,1,
30,3,30,575,8,30,1,30,1,30,5,30,579,8,30,10,30,12,30,582,9,30,1,30,0,3,50,
52,60,31,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,
46,48,50,52,54,56,58,60,0,11,1,0,30,32,2,0,81,81,83,83,3,0,10,12,33,34,51,
57,3,0,34,34,51,57,80,80,1,0,58,59,3,0,60,60,62,62,66,66,1,0,63,65,1,0,61,
62,1,0,67,70,1,0,71,72,1,0,75,79,673,0,65,1,0,0,0,2,70,1,0,0,0,4,159,1,0,
0,0,6,161,1,0,0,0,8,170,1,0,0,0,10,178,1,0,0,0,12,187,1,0,0,0,14,195,1,0,
0,0,16,229,1,0,0,0,18,234,1,0,0,0,20,245,1,0,0,0,22,308,1,0,0,0,24,310,1,
0,0,0,26,319,1,0,0,0,28,322,1,0,0,0,30,334,1,0,0,0,32,351,1,0,0,0,34,361,
1,0,0,0,36,366,1,0,0,0,38,387,1,0,0,0,40,391,1,0,0,0,42,393,1,0,0,0,44,397,
1,0,0,0,46,413,1,0,0,0,48,415,1,0,0,0,50,435,1,0,0,0,52,525,1,0,0,0,54,541,
1,0,0,0,56,550,1,0,0,0,58,557,1,0,0,0,60,574,1,0,0,0,62,64,3,4,2,0,63,62,
1,0,0,0,64,67,1,0,0,0,65,63,1,0,0,0,65,66,1,0,0,0,66,68,1,0,0,0,67,65,1,
0,0,0,68,69,5,0,0,1,69,1,1,0,0,0,70,75,5,80,0,0,71,72,5,1,0,0,72,74,5,80,
0,0,73,71,1,0,0,0,74,77,1,0,0,0,75,73,1,0,0,0,75,76,1,0,0,0,76,3,1,0,0,0,
77,75,1,0,0,0,78,160,3,48,24,0,79,160,3,16,8,0,80,160,3,10,5,0,81,160,3,
12,6,0,82,160,3,28,14,0,83,160,3,30,15,0,84,85,3,38,19,0,85,86,5,2,0,0,86,
160,1,0,0,0,87,160,3,42,21,0,88,90,5,34,0,0,89,91,3,50,25,0,90,89,1,0,0,
0,90,91,1,0,0,0,91,92,1,0,0,0,92,160,5,2,0,0,93,94,5,35,0,0,94,95,5,3,0,
0,95,96,3,50,25,0,96,97,5,4,0,0,97,100,3,4,2,0,98,99,5,36,0,0,99,101,3,4,
2,0,100,98,1,0,0,0,100,101,1,0,0,0,101,160,1,0,0,0,102,103,5,37,0,0,103,
104,5,3,0,0,104,105,3,50,25,0,105,106,5,4,0,0,106,107,3,4,2,0,107,160,1,
0,0,0,108,109,5,38,0,0,109,111,5,3,0,0,110,112,3,40,20,0,111,110,1,0,0,0,
111,112,1,0,0,0,112,113,1,0,0,0,113,115,5,2,0,0,114,116,3,50,25,0,115,114,
1,0,0,0,115,116,1,0,0,0,116,117,1,0,0,0,117,119,5,2,0,0,118,120,3,50,25,
0,119,118,1,0,0,0,119,120,1,0,0,0,120,121,1,0,0,0,121,122,5,4,0,0,122,160,
3,4,2,0,123,124,5,39,0,0,124,160,5,2,0,0,125,126,5,40,0,0,126,160,5,2,0,
0,127,128,5,41,0,0,128,129,5,3,0,0,129,130,3,50,25,0,130,131,5,4,0,0,131,
135,5,5,0,0,132,134,3,6,3,0,133,132,1,0,0,0,134,137,1,0,0,0,135,133,1,0,
0,0,135,136,1,0,0,0,136,139,1,0,0,0,137,135,1,0,0,0,138,140,3,8,4,0,139,
138,1,0,0,0,139,140,1,0,0,0,140,141,1,0,0,0,141,142,5,6,0,0,142,160,1,0,
0,0,143,144,5,49,0,0,144,145,3,48,24,0,145,146,5,50,0,0,146,147,5,3,0,0,
147,148,5,80,0,0,148,149,5,4,0,0,149,150,3,48,24,0,150,160,1,0,0,0,151,152,
5,48,0,0,152,153,3,50,25,0,153,154,5,2,0,0,154,160,1,0,0,0,155,156,3,50,
25,0,156,157,5,2,0,0,157,160,1,0,0,0,158,160,5,2,0,0,159,78,1,0,0,0,159,
79,1,0,0,0,159,80,1,0,0,0,159,81,1,0,0,0,159,82,1,0,0,0,159,83,1,0,0,0,159,
84,1,0,0,0,159,87,1,0,0,0,159,88,1,0,0,0,159,93,1,0,0,0,159,102,1,0,0,0,
159,108,1,0,0,0,159,123,1,0,0,0,159,125,1,0,0,0,159,127,1,0,0,0,159,143,
1,0,0,0,159,151,1,0,0,0,159,155,1,0,0,0,159,158,1,0,0,0,160,5,1,0,0,0,161,
162,5,42,0,0,162,163,3,50,25,0,163,167,5,7,0,0,164,166,3,4,2,0,165,164,1,
0,0,0,166,169,1,0,0,0,167,165,1,0,0,0,167,168,1,0,0,0,168,7,1,0,0,0,169,
167,1,0,0,0,170,171,5,43,0,0,171,175,5,7,0,0,172,174,3,4,2,0,173,172,1,0,
0,0,174,177,1,0,0,0,175,173,1,0,0,0,175,176,1,0,0,0,176,9,1,0,0,0,177,175,
1,0,0,0,178,179,5,28,0,0,179,180,5,8,0,0,180,181,5,80,0,0,181,183,5,3,0,
0,182,184,3,44,22,0,183,182,1,0,0,0,183,184,1,0,0,0,184,185,1,0,0,0,185,
186,5,4,0,0,186,11,1,0,0,0,187,188,5,22,0,0,188,189,5,80,0,0,189,191,5,5,
0,0,190,192,3,14,7,0,191,190,1,0,0,0,191,192,1,0,0,0,192,193,1,0,0,0,193,
194,5,6,0,0,194,13,1,0,0,0,195,200,5,80,0,0,196,197,5,9,0,0,197,199,5,80,
0,0,198,196,1,0,0,0,199,202,1,0,0,0,200,198,1,0,0,0,200,201,1,0,0,0,201,
15,1,0,0,0,202,200,1,0,0,0,203,205,3,24,12,0,204,203,1,0,0,0,204,205,1,0,
0,0,205,206,1,0,0,0,206,207,5,28,0,0,207,210,3,2,1,0,208,209,5,7,0,0,209,
211,3,2,1,0,210,208,1,0,0,0,210,211,1,0,0,0,211,212,1,0,0,0,212,213,5,5,
0,0,213,214,3,18,9,0,214,215,5,6,0,0,215,230,1,0,0,0,216,218,3,24,12,0,217,
216,1,0,0,0,217,218,1,0,0,0,218,219,1,0,0,0,219,220,5,28,0,0,220,223,3,2,
1,0,221,222,5,29,0,0,222,224,3,2,1,0,223,221,1,0,0,0,223,224,1,0,0,0,224,
225,1,0,0,0,225,226,5,5,0,0,226,227,3,18,9,0,227,228,5,6,0,0,228,230,1,0,
0,0,229,204,1,0,0,0,229,217,1,0,0,0,230,17,1,0,0,0,231,233,3,20,10,0,232,
231,1,0,0,0,233,236,1,0,0,0,234,232,1,0,0,0,234,235,1,0,0,0,235,19,1,0,0,
0,236,234,1,0,0,0,237,239,3,24,12,0,238,237,1,0,0,0,238,239,1,0,0,0,239,
241,1,0,0,0,240,242,3,26,13,0,241,240,1,0,0,0,241,242,1,0,0,0,242,243,1,
0,0,0,243,246,3,22,11,0,244,246,3,30,15,0,245,238,1,0,0,0,245,244,1,0,0,
0,246,21,1,0,0,0,247,248,5,17,0,0,248,249,3,60,30,0,249,252,5,80,0,0,250,
251,5,75,0,0,251,253,3,50,25,0,252,250,1,0,0,0,252,253,1,0,0,0,253,254,1,
0,0,0,254,255,5,2,0,0,255,309,1,0,0,0,256,257,5,19,0,0,257,258,3,60,30,0,
258,261,5,80,0,0,259,260,5,75,0,0,260,262,3,50,25,0,261,259,1,0,0,0,261,
262,1,0,0,0,262,263,1,0,0,0,263,264,5,2,0,0,264,309,1,0,0,0,265,266,5,18,
0,0,266,269,5,80,0,0,267,268,5,7,0,0,268,270,3,60,30,0,269,267,1,0,0,0,269,
270,1,0,0,0,270,271,1,0,0,0,271,272,5,75,0,0,272,273,3,50,25,0,273,274,5,
2,0,0,274,309,1,0,0,0,275,276,5,20,0,0,276,277,5,33,0,0,277,278,5,80,0,0,
278,280,5,3,0,0,279,281,3,44,22,0,280,279,1,0,0,0,280,281,1,0,0,0,281,282,
1,0,0,0,282,283,5,4,0,0,283,309,3,48,24,0,284,285,5,21,0,0,285,286,5,33,
0,0,286,287,5,80,0,0,287,289,5,3,0,0,288,290,3,44,22,0,289,288,1,0,0,0,289,
290,1,0,0,0,290,291,1,0,0,0,291,292,5,4,0,0,292,309,5,2,0,0,293,294,5,33,
0,0,294,295,5,80,0,0,295,297,5,3,0,0,296,298,3,44,22,0,297,296,1,0,0,0,297,
298,1,0,0,0,298,299,1,0,0,0,299,300,5,4,0,0,300,309,3,48,24,0,301,302,5,
80,0,0,302,304,5,3,0,0,303,305,3,44,22,0,304,303,1,0,0,0,304,305,1,0,0,0,
305,306,1,0,0,0,306,307,5,4,0,0,307,309,3,48,24,0,308,247,1,0,0,0,308,256,
1,0,0,0,308,265,1,0,0,0,308,275,1,0,0,0,308,284,1,0,0,0,308,293,1,0,0,0,
308,301,1,0,0,0,309,23,1,0,0,0,310,311,5,8,0,0,311,317,5,80,0,0,312,314,
5,3,0,0,313,315,3,58,29,0,314,313,1,0,0,0,314,315,1,0,0,0,315,316,1,0,0,
0,316,318,5,4,0,0,317,312,1,0,0,0,317,318,1,0,0,0,318,25,1,0,0,0,319,320,
7,0,0,0,320,27,1,0,0,0,321,323,3,24,12,0,322,321,1,0,0,0,322,323,1,0,0,0,
323,324,1,0,0,0,324,325,5,33,0,0,325,326,5,80,0,0,326,328,5,3,0,0,327,329,
3,44,22,0,328,327,1,0,0,0,328,329,1,0,0,0,329,330,1,0,0,0,330,331,5,4,0,
0,331,332,3,48,24,0,332,29,1,0,0,0,333,335,3,24,12,0,334,333,1,0,0,0,334,
335,1,0,0,0,335,336,1,0,0,0,336,337,5,25,0,0,337,338,5,33,0,0,338,339,5,
80,0,0,339,341,5,3,0,0,340,342,3,44,22,0,341,340,1,0,0,0,341,342,1,0,0,0,
342,343,1,0,0,0,343,344,5,4,0,0,344,346,5,3,0,0,345,347,3,32,16,0,346,345,
1,0,0,0,346,347,1,0,0,0,347,348,1,0,0,0,348,349,5,4,0,0,349,350,5,2,0,0,
350,31,1,0,0,0,351,356,3,34,17,0,352,353,5,9,0,0,353,355,3,34,17,0,354,352,
1,0,0,0,355,358,1,0,0,0,356,354,1,0,0,0,356,357,1,0,0,0,357,33,1,0,0,0,358,
356,1,0,0,0,359,362,5,80,0,0,360,362,3,36,18,0,361,359,1,0,0,0,361,360,1,
0,0,0,362,363,1,0,0,0,363,364,5,75,0,0,364,365,7,1,0,0,365,35,1,0,0,0,366,
367,7,2,0,0,367,37,1,0,0,0,368,369,3,60,30,0,369,372,5,80,0,0,370,371,5,
75,0,0,371,373,3,50,25,0,372,370,1,0,0,0,372,373,1,0,0,0,373,388,1,0,0,0,
374,375,5,17,0,0,375,378,5,80,0,0,376,377,5,75,0,0,377,379,3,50,25,0,378,
376,1,0,0,0,378,379,1,0,0,0,379,388,1,0,0,0,380,381,5,80,0,0,381,382,5,7,
0,0,382,385,3,60,30,0,383,384,5,75,0,0,384,386,3,50,25,0,385,383,1,0,0,0,
385,386,1,0,0,0,386,388,1,0,0,0,387,368,1,0,0,0,387,374,1,0,0,0,387,380,
1,0,0,0,388,39,1,0,0,0,389,392,3,38,19,0,390,392,3,50,25,0,391,389,1,0,0,
0,391,390,1,0,0,0,392,41,1,0,0,0,393,394,5,26,0,0,394,395,5,83,0,0,395,396,
5,2,0,0,396,43,1,0,0,0,397,402,3,46,23,0,398,399,5,9,0,0,399,401,3,46,23,
0,400,398,1,0,0,0,401,404,1,0,0,0,402,400,1,0,0,0,402,403,1,0,0,0,403,45,
1,0,0,0,404,402,1,0,0,0,405,406,3,60,30,0,406,407,5,80,0,0,407,414,1,0,0,
0,408,411,7,3,0,0,409,410,5,7,0,0,410,412,3,60,30,0,411,409,1,0,0,0,411,
412,1,0,0,0,412,414,1,0,0,0,413,405,1,0,0,0,413,408,1,0,0,0,414,47,1,0,0,
0,415,419,5,5,0,0,416,418,3,4,2,0,417,416,1,0,0,0,418,421,1,0,0,0,419,417,
1,0,0,0,419,420,1,0,0,0,420,422,1,0,0,0,421,419,1,0,0,0,422,423,5,6,0,0,
423,49,1,0,0,0,424,425,6,25,-1,0,425,426,5,3,0,0,426,427,3,60,30,0,427,428,
5,4,0,0,428,429,3,50,25,15,429,436,1,0,0,0,430,431,7,4,0,0,431,436,3,50,
25,11,432,433,7,5,0,0,433,436,3,50,25,10,434,436,3,52,26,0,435,424,1,0,0,
0,435,430,1,0,0,0,435,432,1,0,0,0,435,434,1,0,0,0,436,476,1,0,0,0,437,438,
10,12,0,0,438,439,5,15,0,0,439,475,3,50,25,13,440,441,10,8,0,0,441,442,7,
6,0,0,442,475,3,50,25,9,443,444,10,7,0,0,444,445,7,7,0,0,445,475,3,50,25,
8,446,447,10,6,0,0,447,448,7,8,0,0,448,475,3,50,25,7,449,450,10,5,0,0,450,
451,7,9,0,0,451,475,3,50,25,6,452,453,10,4,0,0,453,454,5,73,0,0,454,475,
3,50,25,5,455,456,10,3,0,0,456,457,5,74,0,0,457,475,3,50,25,4,458,459,10,
2,0,0,459,460,7,10,0,0,460,475,3,50,25,2,461,462,10,14,0,0,462,463,5,13,
0,0,463,464,3,50,25,0,464,465,5,14,0,0,465,475,1,0,0,0,466,467,10,13,0,0,
467,475,7,4,0,0,468,469,10,9,0,0,469,472,5,23,0,0,470,473,3,2,1,0,471,473,
3,60,30,0,472,470,1,0,0,0,472,471,1,0,0,0,473,475,1,0,0,0,474,437,1,0,0,
0,474,440,1,0,0,0,474,443,1,0,0,0,474,446,1,0,0,0,474,449,1,0,0,0,474,452,
1,0,0,0,474,455,1,0,0,0,474,458,1,0,0,0,474,461,1,0,0,0,474,466,1,0,0,0,
474,468,1,0,0,0,475,478,1,0,0,0,476,474,1,0,0,0,476,477,1,0,0,0,477,51,1,
0,0,0,478,476,1,0,0,0,479,480,6,26,-1,0,480,481,5,3,0,0,481,482,3,50,25,
0,482,483,5,4,0,0,483,526,1,0,0,0,484,486,5,13,0,0,485,487,3,58,29,0,486,
485,1,0,0,0,486,487,1,0,0,0,487,488,1,0,0,0,488,526,5,14,0,0,489,491,5,5,
0,0,490,492,3,58,29,0,491,490,1,0,0,0,491,492,1,0,0,0,492,493,1,0,0,0,493,
526,5,6,0,0,494,526,5,81,0,0,495,526,5,82,0,0,496,526,5,83,0,0,497,526,5,
45,0,0,498,526,5,46,0,0,499,526,5,47,0,0,500,526,5,80,0,0,501,502,5,24,0,
0,502,504,5,3,0,0,503,505,3,58,29,0,504,503,1,0,0,0,504,505,1,0,0,0,505,
506,1,0,0,0,506,526,5,4,0,0,507,526,3,56,28,0,508,509,5,44,0,0,509,510,3,
2,1,0,510,512,5,3,0,0,511,513,3,58,29,0,512,511,1,0,0,0,512,513,1,0,0,0,
513,514,1,0,0,0,514,516,5,4,0,0,515,517,3,54,27,0,516,515,1,0,0,0,516,517,
1,0,0,0,517,526,1,0,0,0,518,519,5,33,0,0,519,521,5,3,0,0,520,522,3,44,22,
0,521,520,1,0,0,0,521,522,1,0,0,0,522,523,1,0,0,0,523,524,5,4,0,0,524,526,
3,48,24,0,525,479,1,0,0,0,525,484,1,0,0,0,525,489,1,0,0,0,525,494,1,0,0,
0,525,495,1,0,0,0,525,496,1,0,0,0,525,497,1,0,0,0,525,498,1,0,0,0,525,499,
1,0,0,0,525,500,1,0,0,0,525,501,1,0,0,0,525,507,1,0,0,0,525,508,1,0,0,0,
525,518,1,0,0,0,526,538,1,0,0,0,527,528,10,4,0,0,528,530,5,3,0,0,529,531,
3,58,29,0,530,529,1,0,0,0,530,531,1,0,0,0,531,532,1,0,0,0,532,537,5,4,0,
0,533,534,10,3,0,0,534,535,5,1,0,0,535,537,5,80,0,0,536,527,1,0,0,0,536,
533,1,0,0,0,537,540,1,0,0,0,538,536,1,0,0,0,538,539,1,0,0,0,539,53,1,0,0,
0,540,538,1,0,0,0,541,545,5,5,0,0,542,544,3,20,10,0,543,542,1,0,0,0,544,
547,1,0,0,0,545,543,1,0,0,0,545,546,1,0,0,0,546,548,1,0,0,0,547,545,1,0,
0,0,548,549,5,6,0,0,549,55,1,0,0,0,550,551,5,27,0,0,551,553,5,3,0,0,552,
554,3,58,29,0,553,552,1,0,0,0,553,554,1,0,0,0,554,555,1,0,0,0,555,556,5,
4,0,0,556,57,1,0,0,0,557,562,3,50,25,0,558,559,5,9,0,0,559,561,3,50,25,0,
560,558,1,0,0,0,561,564,1,0,0,0,562,560,1,0,0,0,562,563,1,0,0,0,563,59,1,
0,0,0,564,562,1,0,0,0,565,566,6,30,-1,0,566,575,5,51,0,0,567,575,5,52,0,
0,568,575,5,53,0,0,569,575,5,54,0,0,570,575,5,55,0,0,571,575,5,56,0,0,572,
575,5,57,0,0,573,575,3,2,1,0,574,565,1,0,0,0,574,567,1,0,0,0,574,568,1,0,
0,0,574,569,1,0,0,0,574,570,1,0,0,0,574,571,1,0,0,0,574,572,1,0,0,0,574,
573,1,0,0,0,575,580,1,0,0,0,576,577,10,1,0,0,577,579,5,16,0,0,578,576,1,
0,0,0,579,582,1,0,0,0,580,578,1,0,0,0,580,581,1,0,0,0,581,61,1,0,0,0,582,
580,1,0,0,0,69,65,75,90,100,111,115,119,135,139,159,167,175,183,191,200,
204,210,217,223,229,234,238,241,245,252,261,269,280,289,297,304,308,314,
317,322,328,334,341,346,356,361,372,378,385,387,391,402,411,413,419,435,
472,474,476,486,491,504,512,516,521,525,530,536,538,545,553,562,574,580];


const atn = new antlr4.atn.ATNDeserializer().deserialize(serializedATN);

const decisionsToDFA = atn.decisionToState.map( (ds, index) => new antlr4.dfa.DFA(ds, index) );

const sharedContextCache = new antlr4.atn.PredictionContextCache();

export default class TzdLangParser extends antlr4.Parser {

    static grammarFileName = "TzdLang.g4";
    static literalNames = [ null, "'.'", "';'", "'('", "')'", "'{'", "'}'", 
                            "':'", "'@'", "','", "'type'", "'dll'", "'prototype'", 
                            "'['", "']'", "'^'", "'[]'", "'var'", "'const'", 
                            "'let'", "'static'", "'abstract'", "'enum'", 
                            "'in'", "'super'", "'native'", "'import'", null, 
                            "'class'", "'extends'", "'public'", "'private'", 
                            "'protected'", "'fun'", null, "'if'", "'else'", 
                            "'while'", "'for'", "'break'", "'continue'", 
                            "'switch'", "'case'", "'default'", "'new'", 
                            "'true'", "'false'", "'null'", "'throw'", "'try'", 
                            "'catch'", "'int'", "'float'", "'string'", "'bool'", 
                            "'void'", null, null, "'++'", "'--'", "'gxxx'", 
                            "'+'", "'-'", "'*'", "'/'", "'%'", "'!'", "'>='", 
                            "'<='", "'>'", "'<'", "'=='", "'!='", "'&&'", 
                            "'||'", "'='", "'+='", "'-='", "'*='", "'/='" ];
    static symbolicNames = [ null, null, null, null, null, null, null, null, 
                             null, null, null, null, null, null, null, null, 
                             null, "KW_VAR", "KW_CONST", "KW_LET", "KW_STATIC", 
                             "KW_ABSTRACT", "KW_ENUM", "KW_IN", "KW_SUPER", 
                             "KW_NATIVE", "KW_IMPORT", "KW_PRINT", "KW_CLASS", 
                             "KW_EXTENDS", "KW_PUBLIC", "KW_PRIVATE", "KW_PROTECTED", 
                             "KW_FUN", "KW_RET", "KW_IF", "KW_ELSE", "KW_WHILE", 
                             "KW_FOR", "KW_BREAK", "KW_CONTINUE", "KW_SWITCH", 
                             "KW_CASE", "KW_DEFAULT", "KW_NEW", "KW_TRUE", 
                             "KW_FALSE", "KW_NULL", "KW_THROW", "KW_TRY", 
                             "KW_CATCH", "T_INT", "T_FLOAT", "T_STRING", 
                             "T_BOOL", "T_VOID", "T_PTR", "T_FUNCTION", 
                             "INC", "DEC", "GXXX", "PLUS", "MINUS", "MUL", 
                             "DIV", "MOD", "NOT", "GE", "LE", "GT", "LT", 
                             "EEQ", "NEQ", "AND", "OR", "ASSIGN", "PLUS_ASSIGN", 
                             "MIN_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN", "IDENTIFIER", 
                             "INTEGER", "FLOAT", "STRING", "LINE_COMMENT", 
                             "BLOCK_COMMENT", "WS" ];
    static ruleNames = [ "program", "qualifiedName", "statement", "switchCase", 
                         "switchDefault", "annotationDeclaration", "enumDeclaration", 
                         "enumList", "classDeclaration", "classBody", "classMember", 
                         "memberDecl", "annotationUsage", "accessModifier", 
                         "functionDeclaration", "nativeFunctionDeclaration", 
                         "nativeAttrList", "nativeAttr", "nativePropKey", 
                         "variableDeclaration", "forInit", "importStatement", 
                         "paramList", "param", "block", "expression", "atom", 
                         "classOverrideBlock", "printFunction", "exprList", 
                         "typeType" ];

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
    	case 30:
    	    		return this.typeType_sempred(localctx, predIndex);
        default:
            throw "No predicate with index:" + ruleIndex;
       }
    }

    expression_sempred(localctx, predIndex) {
    	switch(predIndex) {
    		case 0:
    			return this.precpred(this._ctx, 12);
    		case 1:
    			return this.precpred(this._ctx, 8);
    		case 2:
    			return this.precpred(this._ctx, 7);
    		case 3:
    			return this.precpred(this._ctx, 6);
    		case 4:
    			return this.precpred(this._ctx, 5);
    		case 5:
    			return this.precpred(this._ctx, 4);
    		case 6:
    			return this.precpred(this._ctx, 3);
    		case 7:
    			return this.precpred(this._ctx, 2);
    		case 8:
    			return this.precpred(this._ctx, 14);
    		case 9:
    			return this.precpred(this._ctx, 13);
    		case 10:
    			return this.precpred(this._ctx, 9);
    		default:
    			throw "No predicate with index:" + predIndex;
    	}
    };

    atom_sempred(localctx, predIndex) {
    	switch(predIndex) {
    		case 11:
    			return this.precpred(this._ctx, 4);
    		case 12:
    			return this.precpred(this._ctx, 3);
    		default:
    			throw "No predicate with index:" + predIndex;
    	}
    };

    typeType_sempred(localctx, predIndex) {
    	switch(predIndex) {
    		case 13:
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
	        this.state = 65;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 524427564) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 805173751) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	            this.state = 62;
	            this.statement();
	            this.state = 67;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	        this.state = 68;
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
	        this.state = 70;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 75;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,1,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                this.state = 71;
	                this.match(TzdLangParser.T__0);
	                this.state = 72;
	                this.match(TzdLangParser.IDENTIFIER); 
	            }
	            this.state = 77;
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
	        this.state = 159;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,9,this._ctx);
	        switch(la_) {
	        case 1:
	            localctx = new BlockStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 1);
	            this.state = 78;
	            this.block();
	            break;

	        case 2:
	            localctx = new ClassDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 2);
	            this.state = 79;
	            this.classDeclaration();
	            break;

	        case 3:
	            localctx = new AnnotationDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 3);
	            this.state = 80;
	            this.annotationDeclaration();
	            break;

	        case 4:
	            localctx = new EnumDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 4);
	            this.state = 81;
	            this.enumDeclaration();
	            break;

	        case 5:
	            localctx = new FunDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 5);
	            this.state = 82;
	            this.functionDeclaration();
	            break;

	        case 6:
	            localctx = new NativeFunDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 6);
	            this.state = 83;
	            this.nativeFunctionDeclaration();
	            break;

	        case 7:
	            localctx = new VarDeclStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 7);
	            this.state = 84;
	            this.variableDeclaration();
	            this.state = 85;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 8:
	            localctx = new ImportStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 8);
	            this.state = 87;
	            this.importStatement();
	            break;

	        case 9:
	            localctx = new ReturnStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 9);
	            this.state = 88;
	            this.match(TzdLangParser.KW_RET);
	            this.state = 90;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 89;
	                this.expression(0);
	            }

	            this.state = 92;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 10:
	            localctx = new IfStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 10);
	            this.state = 93;
	            this.match(TzdLangParser.KW_IF);
	            this.state = 94;
	            this.match(TzdLangParser.T__2);
	            this.state = 95;
	            this.expression(0);
	            this.state = 96;
	            this.match(TzdLangParser.T__3);
	            this.state = 97;
	            this.statement();
	            this.state = 100;
	            this._errHandler.sync(this);
	            var la_ = this._interp.adaptivePredict(this._input,3,this._ctx);
	            if(la_===1) {
	                this.state = 98;
	                this.match(TzdLangParser.KW_ELSE);
	                this.state = 99;
	                this.statement();

	            }
	            break;

	        case 11:
	            localctx = new WhileStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 11);
	            this.state = 102;
	            this.match(TzdLangParser.KW_WHILE);
	            this.state = 103;
	            this.match(TzdLangParser.T__2);
	            this.state = 104;
	            this.expression(0);
	            this.state = 105;
	            this.match(TzdLangParser.T__3);
	            this.state = 106;
	            this.statement();
	            break;

	        case 12:
	            localctx = new ForStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 12);
	            this.state = 108;
	            this.match(TzdLangParser.KW_FOR);
	            this.state = 109;
	            this.match(TzdLangParser.T__2);
	            this.state = 111;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151134248) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 805074945) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 110;
	                this.forInit();
	            }

	            this.state = 113;
	            this.match(TzdLangParser.T__1);
	            this.state = 115;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 114;
	                localctx.cond = this.expression(0);
	            }

	            this.state = 117;
	            this.match(TzdLangParser.T__1);
	            this.state = 119;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 118;
	                localctx.step = this.expression(0);
	            }

	            this.state = 121;
	            this.match(TzdLangParser.T__3);
	            this.state = 122;
	            this.statement();
	            break;

	        case 13:
	            localctx = new BreakStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 13);
	            this.state = 123;
	            this.match(TzdLangParser.KW_BREAK);
	            this.state = 124;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 14:
	            localctx = new ContinueStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 14);
	            this.state = 125;
	            this.match(TzdLangParser.KW_CONTINUE);
	            this.state = 126;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 15:
	            localctx = new SwitchStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 15);
	            this.state = 127;
	            this.match(TzdLangParser.KW_SWITCH);
	            this.state = 128;
	            this.match(TzdLangParser.T__2);
	            this.state = 129;
	            this.expression(0);
	            this.state = 130;
	            this.match(TzdLangParser.T__3);
	            this.state = 131;
	            this.match(TzdLangParser.T__4);
	            this.state = 135;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            while(_la===42) {
	                this.state = 132;
	                this.switchCase();
	                this.state = 137;
	                this._errHandler.sync(this);
	                _la = this._input.LA(1);
	            }
	            this.state = 139;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===43) {
	                this.state = 138;
	                this.switchDefault();
	            }

	            this.state = 141;
	            this.match(TzdLangParser.T__5);
	            break;

	        case 16:
	            localctx = new TryCatchStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 16);
	            this.state = 143;
	            this.match(TzdLangParser.KW_TRY);
	            this.state = 144;
	            this.block();
	            this.state = 145;
	            this.match(TzdLangParser.KW_CATCH);
	            this.state = 146;
	            this.match(TzdLangParser.T__2);
	            this.state = 147;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 148;
	            this.match(TzdLangParser.T__3);
	            this.state = 149;
	            this.block();
	            break;

	        case 17:
	            localctx = new ThrowStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 17);
	            this.state = 151;
	            this.match(TzdLangParser.KW_THROW);
	            this.state = 152;
	            this.expression(0);
	            this.state = 153;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 18:
	            localctx = new ExprStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 18);
	            this.state = 155;
	            this.expression(0);
	            this.state = 156;
	            this.match(TzdLangParser.T__1);
	            break;

	        case 19:
	            localctx = new EmptyStmtContext(this, localctx);
	            this.enterOuterAlt(localctx, 19);
	            this.state = 158;
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
	        this.state = 161;
	        this.match(TzdLangParser.KW_CASE);
	        this.state = 162;
	        this.expression(0);
	        this.state = 163;
	        this.match(TzdLangParser.T__6);
	        this.state = 167;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 524427564) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 805173751) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	            this.state = 164;
	            this.statement();
	            this.state = 169;
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
	        this.state = 170;
	        this.match(TzdLangParser.KW_DEFAULT);
	        this.state = 171;
	        this.match(TzdLangParser.T__6);
	        this.state = 175;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 524427564) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 805173751) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	            this.state = 172;
	            this.statement();
	            this.state = 177;
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
	        this.state = 178;
	        this.match(TzdLangParser.KW_CLASS);
	        this.state = 179;
	        this.match(TzdLangParser.T__7);
	        this.state = 180;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 181;
	        this.match(TzdLangParser.T__2);
	        this.state = 183;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	            this.state = 182;
	            this.paramList();
	        }

	        this.state = 185;
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
	        this.state = 187;
	        this.match(TzdLangParser.KW_ENUM);
	        this.state = 188;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 189;
	        this.match(TzdLangParser.T__4);
	        this.state = 191;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===80) {
	            this.state = 190;
	            this.enumList();
	        }

	        this.state = 193;
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
	        this.state = 195;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 200;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 196;
	            this.match(TzdLangParser.T__8);
	            this.state = 197;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 202;
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
	        this.state = 229;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,19,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 204;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===8) {
	                this.state = 203;
	                this.annotationUsage();
	            }

	            this.state = 206;
	            this.match(TzdLangParser.KW_CLASS);
	            this.state = 207;
	            this.qualifiedName();
	            this.state = 210;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===7) {
	                this.state = 208;
	                this.match(TzdLangParser.T__6);
	                this.state = 209;
	                this.qualifiedName();
	            }

	            this.state = 212;
	            this.match(TzdLangParser.T__4);
	            this.state = 213;
	            this.classBody();
	            this.state = 214;
	            this.match(TzdLangParser.T__5);
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 217;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===8) {
	                this.state = 216;
	                this.annotationUsage();
	            }

	            this.state = 219;
	            this.match(TzdLangParser.KW_CLASS);
	            this.state = 220;
	            this.qualifiedName();
	            this.state = 223;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===29) {
	                this.state = 221;
	                this.match(TzdLangParser.KW_EXTENDS);
	                this.state = 222;
	                this.qualifiedName();
	            }

	            this.state = 225;
	            this.match(TzdLangParser.T__4);
	            this.state = 226;
	            this.classBody();
	            this.state = 227;
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
	        this.state = 234;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(((((_la - 8)) & ~0x1f) === 0 && ((1 << (_la - 8)) & 63061505) !== 0) || _la===80) {
	            this.state = 231;
	            this.classMember();
	            this.state = 236;
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
	        this.state = 245;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,23,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 238;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===8) {
	                this.state = 237;
	                this.annotationUsage();
	            }

	            this.state = 241;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 30)) & ~0x1f) === 0 && ((1 << (_la - 30)) & 7) !== 0)) {
	                this.state = 240;
	                this.accessModifier();
	            }

	            this.state = 243;
	            this.memberDecl();
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 244;
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
	        this.state = 308;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 17:
	            localctx = new FieldVarDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 1);
	            this.state = 247;
	            this.match(TzdLangParser.KW_VAR);
	            this.state = 248;
	            this.typeType(0);
	            this.state = 249;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 252;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===75) {
	                this.state = 250;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 251;
	                this.expression(0);
	            }

	            this.state = 254;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 19:
	            localctx = new FieldLetDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 2);
	            this.state = 256;
	            this.match(TzdLangParser.KW_LET);
	            this.state = 257;
	            this.typeType(0);
	            this.state = 258;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 261;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===75) {
	                this.state = 259;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 260;
	                this.expression(0);
	            }

	            this.state = 263;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 18:
	            localctx = new FieldConstDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 3);
	            this.state = 265;
	            this.match(TzdLangParser.KW_CONST);
	            this.state = 266;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 269;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===7) {
	                this.state = 267;
	                this.match(TzdLangParser.T__6);
	                this.state = 268;
	                this.typeType(0);
	            }

	            this.state = 271;
	            this.match(TzdLangParser.ASSIGN);
	            this.state = 272;
	            this.expression(0);
	            this.state = 273;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 20:
	            localctx = new MethodStaticDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 4);
	            this.state = 275;
	            this.match(TzdLangParser.KW_STATIC);
	            this.state = 276;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 277;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 278;
	            this.match(TzdLangParser.T__2);
	            this.state = 280;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	                this.state = 279;
	                this.paramList();
	            }

	            this.state = 282;
	            this.match(TzdLangParser.T__3);
	            this.state = 283;
	            this.block();
	            break;
	        case 21:
	            localctx = new MethodAbstractDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 5);
	            this.state = 284;
	            this.match(TzdLangParser.KW_ABSTRACT);
	            this.state = 285;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 286;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 287;
	            this.match(TzdLangParser.T__2);
	            this.state = 289;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	                this.state = 288;
	                this.paramList();
	            }

	            this.state = 291;
	            this.match(TzdLangParser.T__3);
	            this.state = 292;
	            this.match(TzdLangParser.T__1);
	            break;
	        case 33:
	            localctx = new MethodDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 6);
	            this.state = 293;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 294;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 295;
	            this.match(TzdLangParser.T__2);
	            this.state = 297;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	                this.state = 296;
	                this.paramList();
	            }

	            this.state = 299;
	            this.match(TzdLangParser.T__3);
	            this.state = 300;
	            this.block();
	            break;
	        case 80:
	            localctx = new ConstructorDeclContext(this, localctx);
	            this.enterOuterAlt(localctx, 7);
	            this.state = 301;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 302;
	            this.match(TzdLangParser.T__2);
	            this.state = 304;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	                this.state = 303;
	                this.paramList();
	            }

	            this.state = 306;
	            this.match(TzdLangParser.T__3);
	            this.state = 307;
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
	        this.state = 310;
	        this.match(TzdLangParser.T__7);
	        this.state = 311;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 317;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===3) {
	            this.state = 312;
	            this.match(TzdLangParser.T__2);
	            this.state = 314;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 313;
	                this.exprList();
	            }

	            this.state = 316;
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
	        this.state = 319;
	        _la = this._input.LA(1);
	        if(!(((((_la - 30)) & ~0x1f) === 0 && ((1 << (_la - 30)) & 7) !== 0))) {
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
	        this.state = 322;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===8) {
	            this.state = 321;
	            this.annotationUsage();
	        }

	        this.state = 324;
	        this.match(TzdLangParser.KW_FUN);
	        this.state = 325;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 326;
	        this.match(TzdLangParser.T__2);
	        this.state = 328;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	            this.state = 327;
	            this.paramList();
	        }

	        this.state = 330;
	        this.match(TzdLangParser.T__3);
	        this.state = 331;
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
	        this.state = 334;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(_la===8) {
	            this.state = 333;
	            this.annotationUsage();
	        }

	        this.state = 336;
	        this.match(TzdLangParser.KW_NATIVE);
	        this.state = 337;
	        this.match(TzdLangParser.KW_FUN);
	        this.state = 338;
	        this.match(TzdLangParser.IDENTIFIER);
	        this.state = 339;
	        this.match(TzdLangParser.T__2);
	        this.state = 341;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	            this.state = 340;
	            this.paramList();
	        }

	        this.state = 343;
	        this.match(TzdLangParser.T__3);
	        this.state = 344;
	        this.match(TzdLangParser.T__2);
	        this.state = 346;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if(((((_la - 10)) & ~0x1f) === 0 && ((1 << (_la - 10)) & 25165831) !== 0) || ((((_la - 51)) & ~0x1f) === 0 && ((1 << (_la - 51)) & 536871039) !== 0)) {
	            this.state = 345;
	            this.nativeAttrList();
	        }

	        this.state = 348;
	        this.match(TzdLangParser.T__3);
	        this.state = 349;
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
	        this.state = 351;
	        this.nativeAttr();
	        this.state = 356;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 352;
	            this.match(TzdLangParser.T__8);
	            this.state = 353;
	            this.nativeAttr();
	            this.state = 358;
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
	        this.state = 361;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 80:
	            this.state = 359;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;
	        case 10:
	        case 11:
	        case 12:
	        case 33:
	        case 34:
	        case 51:
	        case 52:
	        case 53:
	        case 54:
	        case 55:
	        case 56:
	        case 57:
	            this.state = 360;
	            this.nativePropKey();
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	        this.state = 363;
	        this.match(TzdLangParser.ASSIGN);
	        this.state = 364;
	        _la = this._input.LA(1);
	        if(!(_la===81 || _la===83)) {
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
	        this.state = 366;
	        _la = this._input.LA(1);
	        if(!((((_la) & ~0x1f) === 0 && ((1 << _la) & 7168) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 33292291) !== 0))) {
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
	        this.state = 387;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,44,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 368;
	            this.typeType(0);
	            this.state = 369;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 372;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===75) {
	                this.state = 370;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 371;
	                this.expression(0);
	            }

	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 374;
	            this.match(TzdLangParser.KW_VAR);
	            this.state = 375;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 378;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===75) {
	                this.state = 376;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 377;
	                this.expression(0);
	            }

	            break;

	        case 3:
	            this.enterOuterAlt(localctx, 3);
	            this.state = 380;
	            this.match(TzdLangParser.IDENTIFIER);
	            this.state = 381;
	            this.match(TzdLangParser.T__6);
	            this.state = 382;
	            this.typeType(0);
	            this.state = 385;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===75) {
	                this.state = 383;
	                this.match(TzdLangParser.ASSIGN);
	                this.state = 384;
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
	        this.state = 391;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,45,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 389;
	            this.variableDeclaration();
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 390;
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
	        this.state = 393;
	        this.match(TzdLangParser.KW_IMPORT);
	        this.state = 394;
	        this.match(TzdLangParser.STRING);
	        this.state = 395;
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
	        this.state = 397;
	        this.param();
	        this.state = 402;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 398;
	            this.match(TzdLangParser.T__8);
	            this.state = 399;
	            this.param();
	            this.state = 404;
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
	        this.state = 413;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,48,this._ctx);
	        switch(la_) {
	        case 1:
	            this.enterOuterAlt(localctx, 1);
	            this.state = 405;
	            this.typeType(0);
	            this.state = 406;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;

	        case 2:
	            this.enterOuterAlt(localctx, 2);
	            this.state = 408;
	            _la = this._input.LA(1);
	            if(!(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80)) {
	            this._errHandler.recoverInline(this);
	            }
	            else {
	            	this._errHandler.reportMatch(this);
	                this.consume();
	            }
	            this.state = 411;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(_la===7) {
	                this.state = 409;
	                this.match(TzdLangParser.T__6);
	                this.state = 410;
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
	        this.state = 415;
	        this.match(TzdLangParser.T__4);
	        this.state = 419;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while((((_la) & ~0x1f) === 0 && ((1 << _la) & 524427564) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 805173751) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	            this.state = 416;
	            this.statement();
	            this.state = 421;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	        this.state = 422;
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
	        this.state = 435;
	        this._errHandler.sync(this);
	        var la_ = this._interp.adaptivePredict(this._input,50,this._ctx);
	        switch(la_) {
	        case 1:
	            localctx = new CastExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;

	            this.state = 425;
	            this.match(TzdLangParser.T__2);
	            this.state = 426;
	            this.typeType(0);
	            this.state = 427;
	            this.match(TzdLangParser.T__3);
	            this.state = 428;
	            this.expression(15);
	            break;

	        case 2:
	            localctx = new PrefixExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 430;
	            _la = this._input.LA(1);
	            if(!(_la===58 || _la===59)) {
	            this._errHandler.recoverInline(this);
	            }
	            else {
	            	this._errHandler.reportMatch(this);
	                this.consume();
	            }
	            this.state = 431;
	            this.expression(11);
	            break;

	        case 3:
	            localctx = new UnaryExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 432;
	            _la = this._input.LA(1);
	            if(!(((((_la - 60)) & ~0x1f) === 0 && ((1 << (_la - 60)) & 69) !== 0))) {
	            this._errHandler.recoverInline(this);
	            }
	            else {
	            	this._errHandler.reportMatch(this);
	                this.consume();
	            }
	            this.state = 433;
	            this.expression(10);
	            break;

	        case 4:
	            localctx = new AtomExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 434;
	            this.atom(0);
	            break;

	        }
	        this._ctx.stop = this._input.LT(-1);
	        this.state = 476;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,53,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                if(this._parseListeners!==null) {
	                    this.triggerExitRuleEvent();
	                }
	                _prevctx = localctx;
	                this.state = 474;
	                this._errHandler.sync(this);
	                var la_ = this._interp.adaptivePredict(this._input,52,this._ctx);
	                switch(la_) {
	                case 1:
	                    localctx = new PowerExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 437;
	                    if (!( this.precpred(this._ctx, 12))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 12)");
	                    }
	                    this.state = 438;
	                    this.match(TzdLangParser.T__14);
	                    this.state = 439;
	                    this.expression(13);
	                    break;

	                case 2:
	                    localctx = new MultiplicativeExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 440;
	                    if (!( this.precpred(this._ctx, 8))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 8)");
	                    }
	                    this.state = 441;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 63)) & ~0x1f) === 0 && ((1 << (_la - 63)) & 7) !== 0))) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 442;
	                    this.expression(9);
	                    break;

	                case 3:
	                    localctx = new AdditiveExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 443;
	                    if (!( this.precpred(this._ctx, 7))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 7)");
	                    }
	                    this.state = 444;
	                    _la = this._input.LA(1);
	                    if(!(_la===61 || _la===62)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 445;
	                    this.expression(8);
	                    break;

	                case 4:
	                    localctx = new RelationalExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 446;
	                    if (!( this.precpred(this._ctx, 6))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 6)");
	                    }
	                    this.state = 447;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 67)) & ~0x1f) === 0 && ((1 << (_la - 67)) & 15) !== 0))) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 448;
	                    this.expression(7);
	                    break;

	                case 5:
	                    localctx = new EqualityExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 449;
	                    if (!( this.precpred(this._ctx, 5))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 5)");
	                    }
	                    this.state = 450;
	                    _la = this._input.LA(1);
	                    if(!(_la===71 || _la===72)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 451;
	                    this.expression(6);
	                    break;

	                case 6:
	                    localctx = new LogicalAndExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 452;
	                    if (!( this.precpred(this._ctx, 4))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 4)");
	                    }
	                    this.state = 453;
	                    this.match(TzdLangParser.AND);
	                    this.state = 454;
	                    this.expression(5);
	                    break;

	                case 7:
	                    localctx = new LogicalOrExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 455;
	                    if (!( this.precpred(this._ctx, 3))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 3)");
	                    }
	                    this.state = 456;
	                    this.match(TzdLangParser.OR);
	                    this.state = 457;
	                    this.expression(4);
	                    break;

	                case 8:
	                    localctx = new AssignmentExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 458;
	                    if (!( this.precpred(this._ctx, 2))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 2)");
	                    }
	                    this.state = 459;
	                    _la = this._input.LA(1);
	                    if(!(((((_la - 75)) & ~0x1f) === 0 && ((1 << (_la - 75)) & 31) !== 0))) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    this.state = 460;
	                    this.expression(2);
	                    break;

	                case 9:
	                    localctx = new IndexExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 461;
	                    if (!( this.precpred(this._ctx, 14))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 14)");
	                    }
	                    this.state = 462;
	                    this.match(TzdLangParser.T__12);
	                    this.state = 463;
	                    this.expression(0);
	                    this.state = 464;
	                    this.match(TzdLangParser.T__13);
	                    break;

	                case 10:
	                    localctx = new PostfixExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 466;
	                    if (!( this.precpred(this._ctx, 13))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 13)");
	                    }
	                    this.state = 467;
	                    _la = this._input.LA(1);
	                    if(!(_la===58 || _la===59)) {
	                    this._errHandler.recoverInline(this);
	                    }
	                    else {
	                    	this._errHandler.reportMatch(this);
	                        this.consume();
	                    }
	                    break;

	                case 11:
	                    localctx = new TypeCheckExprContext(this, new ExpressionContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_expression);
	                    this.state = 468;
	                    if (!( this.precpred(this._ctx, 9))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 9)");
	                    }
	                    this.state = 469;
	                    this.match(TzdLangParser.KW_IN);
	                    this.state = 472;
	                    this._errHandler.sync(this);
	                    var la_ = this._interp.adaptivePredict(this._input,51,this._ctx);
	                    switch(la_) {
	                    case 1:
	                        this.state = 470;
	                        this.qualifiedName();
	                        break;

	                    case 2:
	                        this.state = 471;
	                        this.typeType(0);
	                        break;

	                    }
	                    break;

	                } 
	            }
	            this.state = 478;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,53,this._ctx);
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
	        this.state = 525;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 3:
	            localctx = new ParenExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;

	            this.state = 480;
	            this.match(TzdLangParser.T__2);
	            this.state = 481;
	            this.expression(0);
	            this.state = 482;
	            this.match(TzdLangParser.T__3);
	            break;
	        case 13:
	            localctx = new ArrayLiteralExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 484;
	            this.match(TzdLangParser.T__12);
	            this.state = 486;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 485;
	                this.exprList();
	            }

	            this.state = 488;
	            this.match(TzdLangParser.T__13);
	            break;
	        case 5:
	            localctx = new ArrayLiteralExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 489;
	            this.match(TzdLangParser.T__4);
	            this.state = 491;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 490;
	                this.exprList();
	            }

	            this.state = 493;
	            this.match(TzdLangParser.T__5);
	            break;
	        case 81:
	            localctx = new IntExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 494;
	            this.match(TzdLangParser.INTEGER);
	            break;
	        case 82:
	            localctx = new FloatExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 495;
	            this.match(TzdLangParser.FLOAT);
	            break;
	        case 83:
	            localctx = new StringExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 496;
	            this.match(TzdLangParser.STRING);
	            break;
	        case 45:
	            localctx = new BoolTrueExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 497;
	            this.match(TzdLangParser.KW_TRUE);
	            break;
	        case 46:
	            localctx = new BoolFalseExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 498;
	            this.match(TzdLangParser.KW_FALSE);
	            break;
	        case 47:
	            localctx = new NullExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 499;
	            this.match(TzdLangParser.KW_NULL);
	            break;
	        case 80:
	            localctx = new IdExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 500;
	            this.match(TzdLangParser.IDENTIFIER);
	            break;
	        case 24:
	            localctx = new SuperExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 501;
	            this.match(TzdLangParser.KW_SUPER);
	            this.state = 502;
	            this.match(TzdLangParser.T__2);
	            this.state = 504;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 503;
	                this.exprList();
	            }

	            this.state = 506;
	            this.match(TzdLangParser.T__3);
	            break;
	        case 27:
	            localctx = new PrintFunExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 507;
	            this.printFunction();
	            break;
	        case 44:
	            localctx = new NewExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 508;
	            this.match(TzdLangParser.KW_NEW);
	            this.state = 509;
	            this.qualifiedName();
	            this.state = 510;
	            this.match(TzdLangParser.T__2);
	            this.state = 512;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                this.state = 511;
	                this.exprList();
	            }

	            this.state = 514;
	            this.match(TzdLangParser.T__3);
	            this.state = 516;
	            this._errHandler.sync(this);
	            var la_ = this._interp.adaptivePredict(this._input,58,this._ctx);
	            if(la_===1) {
	                this.state = 515;
	                this.classOverrideBlock();

	            }
	            break;
	        case 33:
	            localctx = new LambdaExprContext(this, localctx);
	            this._ctx = localctx;
	            _prevctx = localctx;
	            this.state = 518;
	            this.match(TzdLangParser.KW_FUN);
	            this.state = 519;
	            this.match(TzdLangParser.T__2);
	            this.state = 521;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	            if(((((_la - 34)) & ~0x1f) === 0 && ((1 << (_la - 34)) & 16646145) !== 0) || _la===80) {
	                this.state = 520;
	                this.paramList();
	            }

	            this.state = 523;
	            this.match(TzdLangParser.T__3);
	            this.state = 524;
	            this.block();
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	        this._ctx.stop = this._input.LT(-1);
	        this.state = 538;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,63,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                if(this._parseListeners!==null) {
	                    this.triggerExitRuleEvent();
	                }
	                _prevctx = localctx;
	                this.state = 536;
	                this._errHandler.sync(this);
	                var la_ = this._interp.adaptivePredict(this._input,62,this._ctx);
	                switch(la_) {
	                case 1:
	                    localctx = new CallExprContext(this, new AtomContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_atom);
	                    this.state = 527;
	                    if (!( this.precpred(this._ctx, 4))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 4)");
	                    }
	                    this.state = 528;
	                    this.match(TzdLangParser.T__2);
	                    this.state = 530;
	                    this._errHandler.sync(this);
	                    _la = this._input.LA(1);
	                    if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	                        this.state = 529;
	                        this.exprList();
	                    }

	                    this.state = 532;
	                    this.match(TzdLangParser.T__3);
	                    break;

	                case 2:
	                    localctx = new MemberAccessExprContext(this, new AtomContext(this, _parentctx, _parentState));
	                    this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_atom);
	                    this.state = 533;
	                    if (!( this.precpred(this._ctx, 3))) {
	                        throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 3)");
	                    }
	                    this.state = 534;
	                    this.match(TzdLangParser.T__0);
	                    this.state = 535;
	                    this.match(TzdLangParser.IDENTIFIER);
	                    break;

	                } 
	            }
	            this.state = 540;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,63,this._ctx);
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
	        this.state = 541;
	        this.match(TzdLangParser.T__4);
	        this.state = 545;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(((((_la - 8)) & ~0x1f) === 0 && ((1 << (_la - 8)) & 63061505) !== 0) || _la===80) {
	            this.state = 542;
	            this.classMember();
	            this.state = 547;
	            this._errHandler.sync(this);
	            _la = this._input.LA(1);
	        }
	        this.state = 548;
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
	        this.state = 550;
	        this.match(TzdLangParser.KW_PRINT);
	        this.state = 551;
	        this.match(TzdLangParser.T__2);
	        this.state = 553;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        if((((_la) & ~0x1f) === 0 && ((1 << _la) & 151003176) !== 0) || ((((_la - 33)) & ~0x1f) === 0 && ((1 << (_la - 33)) & 771782657) !== 0) || ((((_la - 66)) & ~0x1f) === 0 && ((1 << (_la - 66)) & 245761) !== 0)) {
	            this.state = 552;
	            this.exprList();
	        }

	        this.state = 555;
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
	        this.state = 557;
	        this.expression(0);
	        this.state = 562;
	        this._errHandler.sync(this);
	        _la = this._input.LA(1);
	        while(_la===9) {
	            this.state = 558;
	            this.match(TzdLangParser.T__8);
	            this.state = 559;
	            this.expression(0);
	            this.state = 564;
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


	typeType(_p) {
		if(_p===undefined) {
		    _p = 0;
		}
	    const _parentctx = this._ctx;
	    const _parentState = this.state;
	    let localctx = new TypeTypeContext(this, this._ctx, _parentState);
	    let _prevctx = localctx;
	    const _startState = 60;
	    this.enterRecursionRule(localctx, 60, TzdLangParser.RULE_typeType, _p);
	    try {
	        this.enterOuterAlt(localctx, 1);
	        this.state = 574;
	        this._errHandler.sync(this);
	        switch(this._input.LA(1)) {
	        case 51:
	            this.state = 566;
	            this.match(TzdLangParser.T_INT);
	            break;
	        case 52:
	            this.state = 567;
	            this.match(TzdLangParser.T_FLOAT);
	            break;
	        case 53:
	            this.state = 568;
	            this.match(TzdLangParser.T_STRING);
	            break;
	        case 54:
	            this.state = 569;
	            this.match(TzdLangParser.T_BOOL);
	            break;
	        case 55:
	            this.state = 570;
	            this.match(TzdLangParser.T_VOID);
	            break;
	        case 56:
	            this.state = 571;
	            this.match(TzdLangParser.T_PTR);
	            break;
	        case 57:
	            this.state = 572;
	            this.match(TzdLangParser.T_FUNCTION);
	            break;
	        case 80:
	            this.state = 573;
	            this.qualifiedName();
	            break;
	        default:
	            throw new antlr4.error.NoViableAltException(this);
	        }
	        this._ctx.stop = this._input.LT(-1);
	        this.state = 580;
	        this._errHandler.sync(this);
	        var _alt = this._interp.adaptivePredict(this._input,68,this._ctx)
	        while(_alt!=2 && _alt!=antlr4.atn.ATN.INVALID_ALT_NUMBER) {
	            if(_alt===1) {
	                if(this._parseListeners!==null) {
	                    this.triggerExitRuleEvent();
	                }
	                _prevctx = localctx;
	                localctx = new TypeTypeContext(this, _parentctx, _parentState);
	                this.pushNewRecursionContext(localctx, _startState, TzdLangParser.RULE_typeType);
	                this.state = 576;
	                if (!( this.precpred(this._ctx, 1))) {
	                    throw new antlr4.error.FailedPredicateException(this, "this.precpred(this._ctx, 1)");
	                }
	                this.state = 577;
	                this.match(TzdLangParser.T__15); 
	            }
	            this.state = 582;
	            this._errHandler.sync(this);
	            _alt = this._interp.adaptivePredict(this._input,68,this._ctx);
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
TzdLangParser.T__15 = 16;
TzdLangParser.KW_VAR = 17;
TzdLangParser.KW_CONST = 18;
TzdLangParser.KW_LET = 19;
TzdLangParser.KW_STATIC = 20;
TzdLangParser.KW_ABSTRACT = 21;
TzdLangParser.KW_ENUM = 22;
TzdLangParser.KW_IN = 23;
TzdLangParser.KW_SUPER = 24;
TzdLangParser.KW_NATIVE = 25;
TzdLangParser.KW_IMPORT = 26;
TzdLangParser.KW_PRINT = 27;
TzdLangParser.KW_CLASS = 28;
TzdLangParser.KW_EXTENDS = 29;
TzdLangParser.KW_PUBLIC = 30;
TzdLangParser.KW_PRIVATE = 31;
TzdLangParser.KW_PROTECTED = 32;
TzdLangParser.KW_FUN = 33;
TzdLangParser.KW_RET = 34;
TzdLangParser.KW_IF = 35;
TzdLangParser.KW_ELSE = 36;
TzdLangParser.KW_WHILE = 37;
TzdLangParser.KW_FOR = 38;
TzdLangParser.KW_BREAK = 39;
TzdLangParser.KW_CONTINUE = 40;
TzdLangParser.KW_SWITCH = 41;
TzdLangParser.KW_CASE = 42;
TzdLangParser.KW_DEFAULT = 43;
TzdLangParser.KW_NEW = 44;
TzdLangParser.KW_TRUE = 45;
TzdLangParser.KW_FALSE = 46;
TzdLangParser.KW_NULL = 47;
TzdLangParser.KW_THROW = 48;
TzdLangParser.KW_TRY = 49;
TzdLangParser.KW_CATCH = 50;
TzdLangParser.T_INT = 51;
TzdLangParser.T_FLOAT = 52;
TzdLangParser.T_STRING = 53;
TzdLangParser.T_BOOL = 54;
TzdLangParser.T_VOID = 55;
TzdLangParser.T_PTR = 56;
TzdLangParser.T_FUNCTION = 57;
TzdLangParser.INC = 58;
TzdLangParser.DEC = 59;
TzdLangParser.GXXX = 60;
TzdLangParser.PLUS = 61;
TzdLangParser.MINUS = 62;
TzdLangParser.MUL = 63;
TzdLangParser.DIV = 64;
TzdLangParser.MOD = 65;
TzdLangParser.NOT = 66;
TzdLangParser.GE = 67;
TzdLangParser.LE = 68;
TzdLangParser.GT = 69;
TzdLangParser.LT = 70;
TzdLangParser.EEQ = 71;
TzdLangParser.NEQ = 72;
TzdLangParser.AND = 73;
TzdLangParser.OR = 74;
TzdLangParser.ASSIGN = 75;
TzdLangParser.PLUS_ASSIGN = 76;
TzdLangParser.MIN_ASSIGN = 77;
TzdLangParser.MUL_ASSIGN = 78;
TzdLangParser.DIV_ASSIGN = 79;
TzdLangParser.IDENTIFIER = 80;
TzdLangParser.INTEGER = 81;
TzdLangParser.FLOAT = 82;
TzdLangParser.STRING = 83;
TzdLangParser.LINE_COMMENT = 84;
TzdLangParser.BLOCK_COMMENT = 85;
TzdLangParser.WS = 86;

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
TzdLangParser.RULE_typeType = 30;

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

class UnaryExprContext extends ExpressionContext {

    constructor(parser, ctx) {
        super(parser);
        super.copyFrom(ctx);
    }

	expression() {
	    return this.getTypedRuleContext(ExpressionContext,0);
	};

	MINUS() {
	    return this.getToken(TzdLangParser.MINUS, 0);
	};

	NOT() {
	    return this.getToken(TzdLangParser.NOT, 0);
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
TzdLangParser.TypeTypeContext = TypeTypeContext; 
