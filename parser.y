%{
/***********************************************************************
 * --YOUR GROUP INFO SHOULD GO HERE--
 * 
 *   Interface to the parser module for CSC467 course project.
 * 
 *   Phase 2: Implement context free grammar for source language, and
 *            parse tracing functionality.
 *   Phase 3: Construct the AST for the source language program.
 ***********************************************************************/

/***********************************************************************
 *  C Definitions and external declarations for this module.
 *
 *  Phase 3: Include ast.h if needed, and declarations for other global or
 *           external vars, functions etc. as needed.
 ***********************************************************************/

#include <string.h>
#include "common.h"
//#include "ast.h"
//#include "symbol.h"
//#include "semantic.h"
#define YYERROR_VERBOSE
#define yTRACE(x)    { if (traceParser) fprintf(traceFile, "%s\n", x); }

/* lab1 add const here */
void yyerror(const char* s);    /* what to do in case of error            */
int yylex();              /* procedure for calling lexical analyzer */
extern int yyline;        /* variable holding current line number   */

%}

/***********************************************************************
 *  Yacc/Bison declarations.
 *  Phase 2:
 *    1. Add precedence declarations for operators (after %start declaration)
 *    2. If necessary, add %type declarations for some nonterminals
 *  Phase 3:
 *    1. Add fields to the union below to facilitate the construction of the
 *       AST (the two existing fields allow the lexical analyzer to pass back
 *       semantic info, so they shouldn't be touched).
 *    2. Add <type> modifiers to appropriate %token declarations (using the
 *       fields of the union) so that semantic information can by passed back
 *       by the scanner.
 *    3. Make the %type declarations for the language non-terminals, utilizing
 *       the fields of the union as well.
 ***********************************************************************/

%{
#define YYDEBUG 1
%}


// TODO:Modify me to add more data types
// Can access me from flex useing yyval

%union {
  int intVal;
  float floatVal;
  bool boolVal;
  char *id;
}

// TODO:Replace myToken with your tokens, you can use these tokens in flex
%token
myToken1
myToken2

IDENTIFIER

/* Separators */
/* , */
COMMA
/* ; */
SEMICOLON
/* () */
LBRAC                           /* Left bracket */
RBRAC
/* [] */
LSBRAC                          /* Left small bracket */
RSBRAC
/* {} */
LSCOPE
RSCOPE

/* Math Operators */
ADD
SUB
MUL
DIV
POW

ASSIGN

/* Boolean Operators */
AND
OR
NOT
EQ
NEQ
GT                              /* Greater than */
LT                              /* Less than */
GE                              /* Greater equal */
LE                              /* Less equal */

/* Data types */
T_VOID
T_INT
T_BOOL
T_FLOAT
T_VEC2
T_VEC3
T_VEC4
T_BVEC2
T_BVEC3
T_BVEC4
T_IVEC2
T_IVEC3
T_IVEC4

/* Data values */
V_INT
V_FLOAT
V_BOOL

/* Flow control */
IF
ELSE
WHILE

/* Other keywords */
CONST

%start    program

%%

/***********************************************************************
 *  Yacc/Bison rules
 *  Phase 2:
 *    1. Replace grammar found here with something reflecting the source
 *       language grammar
 *    2. Implement the trace parser option of the compiler
 *  Phase 3:
 *    1. Add code to rules for construction of AST.
 ***********************************************************************/
program
  :   tokens       
  ;
tokens
  :  tokens token  
  |      
  ;
// TODO: replace myToken with the token the you defined.
token
  :     IDENTIFIER
  |     COMMA
  |     SEMICOLON
  |     LBRAC
  |     RBRAC
  |     LSBRAC
  |     RSBRAC
  |     LSCOPE
  |     RSCOPE
  |     ADD
  |     SUB
  |     MUL
  |     DIV
  |     POW
  |     ASSIGN
  |     AND
  |     OR
  |     NOT
  |     EQ
  |     NEQ
  |     GT
  |     LT
  |     GE
  |     LE
  |     T_VOID
  |     T_INT
  |     T_BOOL
  |     T_FLOAT
  |     T_VEC2
  |     T_VEC3
  |     T_VEC4
  |     T_BVEC2
  |     T_BVEC3
  |     T_BVEC4
  |     T_IVEC2
  |     T_IVEC3
  |     T_IVEC4
  |     V_INT
  |     V_FLOAT
  |     V_BOOL
  |     IF
  |     ELSE
  |     WHILE
  |     CONST
  ;


%%

/***********************************************************************ol
 * Extra C code.
 *
 * The given yyerror function should not be touched. You may add helper
 * functions as necessary in subsequent phases.
 ***********************************************************************/
void yyerror(const char* s) {
  if (errorOccurred)
    return;    /* Error has already been reported by scanner */
  else
    errorOccurred = 1;
        
  fprintf(errorFile, "\nPARSER ERROR, LINE %d",yyline);
  if (strcmp(s, "parse error")) {
    if (strncmp(s, "parse error, ", 13))
      fprintf(errorFile, ": %s\n", s);
    else
      fprintf(errorFile, ": %s\n", s+13);
  } else
    fprintf(errorFile, ": Reading token %s\n", yytname[YYTRANSLATE(yychar)]);
}

