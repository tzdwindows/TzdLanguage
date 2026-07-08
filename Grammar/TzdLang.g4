grammar TzdLang;

// --- 1. ??????? ---
program
    : statement* EOF
    ;

// --- 2. ???????? ---
qualifiedName
    : IDENTIFIER ('.' IDENTIFIER)*
    ;

// --- 3. ??? (Statement) ---
statement
    : block                                                                 # BlockStmt
    | classDeclaration                                                      # ClassDeclStmt
    | annotationDeclaration                                                 # AnnotationDeclStmt
    | enumDeclaration                                                       # EnumDeclStmt
    | functionDeclaration                                                   # FunDeclStmt
    | nativeFunctionDeclaration                                             # NativeFunDeclStmt
    | variableDeclaration ';'                                               # VarDeclStmt
    | importStatement                                                       # ImportStmt
    | KW_RET expression? ';'                                                # ReturnStmt
    | KW_IF '(' expression ')' statement (KW_ELSE statement)?               # IfStmt
    | KW_WHILE '(' expression ')' statement                                 # WhileStmt
    | KW_FOR '(' forInit? ';' cond=expression? ';' step=expression? ')' statement # ForStmt
    | KW_BREAK ';'                                                          # BreakStmt
    | KW_CONTINUE ';'                                                       # ContinueStmt
    | KW_SWITCH '(' expression ')' '{' switchCase* switchDefault? '}'     # SwitchStmt
    | KW_TRY block KW_CATCH '(' IDENTIFIER ')' block                       # TryCatchStmt
    | KW_THROW expression ';'                                               # ThrowStmt
    | expression ';'                                                        # ExprStmt
    | ';'                                                                   # EmptyStmt
    ;

switchCase
    : KW_CASE expression ':' statement*
    ;

switchDefault
    : KW_DEFAULT ':' statement*
    ;

// --- 4. OOP ???? ---

// ?????: class @Name(args)
annotationDeclaration
    : KW_CLASS '@' IDENTIFIER '(' paramList? ')' 
    ;

// ??????
enumDeclaration
    : KW_ENUM IDENTIFIER '{' enumList? '}'
    ;
enumList : IDENTIFIER (',' IDENTIFIER)* ;

// ????
// ???: class org.tzd.Test : Parent {} 
// ???: class Test extends org.tzd.Base {}
classDeclaration
    : annotationUsage? KW_CLASS qualifiedName (':' qualifiedName)? '{' classBody '}' 
    | annotationUsage? KW_CLASS qualifiedName (KW_EXTENDS qualifiedName)? '{' classBody '}'
    ;

classBody : classMember* ;

classMember
    : annotationUsage? accessModifier? memberDecl
    | nativeFunctionDeclaration
    ;

memberDecl
    // ??????: var int bb;
    : KW_VAR typeType IDENTIFIER (ASSIGN expression)? ';'            # FieldVarDecl
    // ??????: let int bb; (let????????????????)
    | KW_LET typeType IDENTIFIER (ASSIGN expression)? ';'            # FieldLetDecl
    // ???????: const bb = 0;
    | KW_CONST IDENTIFIER (':' typeType)? ASSIGN expression ';'      # FieldConstDecl
    // ???????
    | KW_STATIC KW_FUN IDENTIFIER '(' paramList? ')' block           # MethodStaticDecl
    // ?????
    | KW_ABSTRACT KW_FUN IDENTIFIER '(' paramList? ')' ';'           # MethodAbstractDecl
    // ???????
    | KW_FUN IDENTIFIER '(' paramList? ')' block                     # MethodDecl
    // ??????: Test() {} (??????????????????????��????)
    | IDENTIFIER '(' paramList? ')' block                            # ConstructorDecl
    ;

annotationUsage : '@' IDENTIFIER ('(' exprList? ')')? ;

accessModifier : KW_PUBLIC | KW_PRIVATE | KW_PROTECTED ;


// ???????
functionDeclaration
    : annotationUsage? KW_FUN IDENTIFIER '(' paramList? ')' block
    ;

// Native ????
nativeFunctionDeclaration
    : annotationUsage? KW_NATIVE KW_FUN IDENTIFIER '(' paramList? ')' 
      '(' nativeAttrList? ')' ';'
    ;
nativeAttrList : nativeAttr (',' nativeAttr)* ;
nativeAttr : (IDENTIFIER | nativePropKey) '=' (STRING | INTEGER) ;
nativePropKey : T_INT | T_STRING | T_FLOAT | T_BOOL | T_VOID | T_PTR | T_FUNCTION | KW_RET | 'type' | 'fun' | 'dll' | 'prototype';

// ??????????? (???????)
variableDeclaration
    : typeType IDENTIFIER ('=' expression)?        // int a = 1;
    | KW_VAR IDENTIFIER ('=' expression)?          // var a = 1; (??????)
    | IDENTIFIER ':' typeType ('=' expression)?    // a : int = 1; (???????)
    ;

forInit : variableDeclaration | expression ;
importStatement : KW_IMPORT STRING ';' ;
paramList : param (',' param)* ;
param : typeType IDENTIFIER
      | (IDENTIFIER | T_INT | T_STRING | T_FLOAT | T_BOOL | T_VOID | T_PTR | T_FUNCTION | KW_RET) (':' typeType)? ;
block : '{' statement* '}' ;

// --- 5. ????? (Expression) ---
expression
    : '(' typeType ')' expression                   # CastExpr
    | expression '[' expression ']'                  # IndexExpr
    | expression (INC | DEC)                         # PostfixExpr
    | expression '^' expression                      # PowerExpr
    | (INC | DEC) expression                         # PrefixExpr
    | (MINUS | NOT | GXXX) expression                # UnaryExpr
    // ??????: obj in org.tzd.Test ?? val in int
    | expression KW_IN (qualifiedName | typeType)    # TypeCheckExpr  
    | expression (MUL | DIV | MOD) expression         # MultiplicativeExpr
    | expression (PLUS | MINUS) expression           # AdditiveExpr
    | expression (GT | LT | GE | LE) expression       # RelationalExpr
    | expression (EEQ | NEQ) expression               # EqualityExpr
    | expression AND expression                      # LogicalAndExpr
    | expression OR expression                       # LogicalOrExpr
    | <assoc=right> expression 
      (ASSIGN | PLUS_ASSIGN | MIN_ASSIGN | MUL_ASSIGN | DIV_ASSIGN) 
      expression                                     # AssignmentExpr
    | atom                                           # AtomExpr
    ;

atom
    : '(' expression ')'                               # ParenExpr
    | '[' exprList? ']'                                # ArrayLiteralExpr
    | '{' exprList? '}'                                # ArrayLiteralExpr
    | INTEGER                                          # IntExpr
    | FLOAT                                            # FloatExpr
    | STRING                                           # StringExpr
    | KW_TRUE                                          # BoolTrueExpr
    | KW_FALSE                                         # BoolFalseExpr
    | KW_NULL                                          # NullExpr
    | IDENTIFIER                                       # IdExpr
    | KW_SUPER '(' exprList? ')'                       # SuperExpr    
    | printFunction                                    # PrintFunExpr
    | atom '(' exprList? ')'                           # CallExpr 
    | atom '.' IDENTIFIER                              # MemberAccessExpr
    // New: new org.tzd.Test() { ... }
    | KW_NEW qualifiedName '(' exprList? ')' classOverrideBlock? # NewExpr 
    | KW_FUN '(' paramList? ')' block                  # LambdaExpr
    ;

classOverrideBlock : '{' classMember* '}' ;

printFunction : KW_PRINT '(' exprList? ')' ;
exprList : expression (',' expression)* ;

// ???????
typeType 
    : T_INT | T_FLOAT | T_STRING | T_BOOL | T_VOID | T_PTR | T_FUNCTION
    | qualifiedName  // ?????????????????? (?? org.tzd.Test)
    | typeType '[]' 
    ;

// --- 6. ??????? (Lexer) ---
KW_VAR : 'var';
KW_CONST : 'const';
KW_LET : 'let';
KW_STATIC : 'static';
KW_ABSTRACT : 'abstract';
KW_ENUM : 'enum';
KW_IN : 'in';
KW_SUPER : 'super';

KW_NATIVE : 'native';
KW_IMPORT : 'import';
KW_PRINT : 'print' | 'printf' | 'out';
KW_CLASS : 'class';
KW_EXTENDS : 'extends';
KW_PUBLIC : 'public';
KW_PRIVATE : 'private';
KW_PROTECTED : 'protected';
KW_FUN : 'fun';
KW_RET : 'return' | 'ret';
KW_IF : 'if';
KW_ELSE : 'else';
KW_WHILE : 'while';
KW_FOR : 'for';
KW_BREAK : 'break';
KW_CONTINUE : 'continue';
KW_SWITCH : 'switch';
KW_CASE : 'case';
KW_DEFAULT : 'default';
KW_NEW : 'new';
KW_TRUE : 'true';
KW_FALSE : 'false';
KW_NULL : 'null';
KW_THROW : 'throw';
KW_TRY : 'try';
KW_CATCH : 'catch';

T_INT : 'int';
T_FLOAT : 'float';
T_STRING : 'string';
T_BOOL : 'bool';
T_VOID : 'void';
T_PTR    : 'ptr' | 'pointer' | 'hwnd';
T_FUNCTION : 'function' | 'fn';

INC : '++'; DEC : '--';
GXXX : 'gxxx';
PLUS : '+'; MINUS : '-'; MUL : '*'; DIV : '/'; MOD : '%'; NOT : '!';
GE : '>='; LE : '<='; GT : '>'; LT : '<'; EEQ : '=='; NEQ : '!=';
AND : '&&'; OR : '||'; ASSIGN : '='; PLUS_ASSIGN : '+='; MIN_ASSIGN : '-=';
MUL_ASSIGN : '*='; DIV_ASSIGN : '/=';

IDENTIFIER : [a-zA-Z_] [a-zA-Z0-9_]* ;
INTEGER : [0-9]+ | '0' [xX] [0-9a-fA-F]+ ;
FLOAT : [0-9]+ '.' [0-9]+ ;
STRING : ('"' .*? '"' | '\'' .*? '\'') ;
LINE_COMMENT : '//' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;
WS : [ \t\r\n]+ -> skip ;