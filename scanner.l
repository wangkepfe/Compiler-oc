%{

#include "lyutils.h"

#define YY_USER_ACTION  { lexer::advance(); }

int yylval_token (int _symbol) {

    // astree node
    yylval = new astree (_symbol, lexer::lloc, yytext);

    // tok file
    fprintf(tokenFile
        , "%4ld  %6d.%-3d  %5d  %-15s  (%s)\n"
        , lexer::lloc.filenr
        , lexer::lloc.linenr
        , lexer::lloc.offset
        , _symbol
        , parser::get_tname(_symbol)
        , yytext);

    return _symbol;
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

CHAR            ('([^\\'\n]|\\[\\'"0nt])')
STRING          (\"([^\\"\n]|\\[\\'"0nt])*\")

BADINDENT       ({DIGIT}({LETTER}|{DIGIT})*)
BADCHAR         (('([^\\'\n]|\\[\\'"0nt]))|('([^'\n])'))
BADSTRING       ((\"([^\\"\n]|\\[\\'"0nt])*)|(\"([^"\n])*\"))

%%

\/\*(.|\n)*\*\/ { }
\/\/.*          { }
[ \t]+          { }
\n              { lexer::newline(); }

"#".*           { lexer::include(); }

{INT}           { return yylval_token (TOK_INTCON); }
{OCT}           { return yylval_token (TOK_INTCON); }
{HEX}           { return yylval_token (TOK_INTCON); }

{CHAR}          { return yylval_token (TOK_CHARCON); }
{STRING}        { return yylval_token (TOK_STRINGCON); }

"[]"            { return yylval_token (TOK_ARRAY); }

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

{IDENT}         { return yylval_token (TOK_IDENT); }

{BADINDENT}     { lexer::badtoken (yytext); }
{BADSTRING}     { lexer::badtoken (yytext); }
{BADCHAR}       { lexer::badchar (*yytext); }
.               { lexer::badchar (*yytext); }

%%
