%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

int yylval_token (int symbol) {
    if(yylval != nullptr) delete yylval;
    yylval = new astree (symbol, lexer::lloc, yytext);
    return symbol;
}

%}

%option 8bit
%option debug
%option nodefault
%option nounput
%option noyywrap
%option verbose
%option warn

LETTER          [A-Za-z_]
DIGIT           [0-9]

IDENT           ({LETTER}({LETTER}|{DIGIT})*)

INT             ([1-9]{DIGIT}*)
OCT             (0[0-7]*)
HEX             ((0x|0X)({DIGIT}|[A-Fa-f])+)

CHAR            (’([^\\’\n]|\\[\\’"0nt])’)
STRING          (\"([^\\"\n]|\\[\\’"0nt])*\")

BADINDENT       ({DIGIT}({LETTER}|{DIGIT})*)
BADCHAR         ((’([^\\’\n]|\\[\\’"0nt]))|(’([^’\n])’))
BADSTRING       ((\"([^\\"\n]|\\[\\’"0nt])*)|(\"([^"\n])*\"))

%%

\/\*(.|\n)*\*\/ { }
\/\/.*          { }
[ \t]+          { }
\n              { lexer::newline(); }

"#"[ \t]+{INT}[ \t]+{STRING} { return yylval_token (TOK_OUT_DIREC);  }

"#".*           { lexer::include(); }

{IDENT}         { return yylval_token (TOK_IDENT); }

{INT}           { return yylval_token (TOK_INT); }
{OCT}           { return yylval_token (TOK_OCT); }
{HEX}           { return yylval_token (TOK_HEX); }

{CHAR}          { return yylval_token (TOK_CHAR); }
{STRING}        { return yylval_token (TOK_STRING); }

"[]"            { return yylval_token (TOK_BRACKETS); }

"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"%"             { return yylval_token ('%'); }
"!"             { return yylval_token ('!'); }
"("             { return yylval_token ('('); }
")"             { return yylval_token (')'); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }
"{"             { return yylval_token ('{'); }
"}"             { return yylval_token ('}'); }
";"             { return yylval_token (';'); }
","             { return yylval_token (','); }
"="             { return yylval_token ('='); }

"=="            { return yylval_token (TOK_EQ); }
"!="            { return yylval_token (TOK_NE); }
"<="            { return yylval_token (TOK_LE); }
">="            { return yylval_token (TOK_GE); }

"if"            { return yylval_token (TOK_IF); }
"else"          { return yylval_token (TOK_ELSE); }
"while"         { return yylval_token (TOK_WHILE); }
"return"        { return yylval_token (TOK_RETURN); }
"int"           { return yylval_token (TOK_INT); }
"string"        { return yylval_token (TOK_STRING); }
"struct"        { return yylval_token (TOK_STRUCT); }
"void"          { return yylval_token (TOK_VOID); }
"new"           { return yylval_token (TOK_NEW); }
"null"          { return yylval_token (TOK_NULL); }
"not"           { return yylval_token (TOK_NOT); }

{BADINDENT}     { lexer::badtoken (yytext); }
{BADCHAR}       { lexer::badtoken (yytext); }
{BADSTRING}     { lexer::badtoken (yytext); }
.               { lexer::badchar (*yytext); }

%%