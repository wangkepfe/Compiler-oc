%{

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "lyutils.h"

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%destructor { destroy ($$); } <>

%initial-action {
   parser::root = new astree (TOK_ROOT, {0, 0, 0}, "");
}

%token TOK_ROOT

%token TOK_VOID TOK_INT TOK_STRING
%token TOK_OCT TOK_HEX

%token TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_IF TOK_ELSE TOK_WHILE 
 
%token TOK_STRUCT TOK_FIELD

%token TOK_NULL TOK_NEW TOK_ARRAY

%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_NOT
%token TOK_POS TOK_NEG

%token TOK_IDENT TOK_TYPEID TOK_DECLID TOK_VARDECL

%token TOK_NEWARRAY TOK_NEWSTR
 
%token TOK_PROTO TOK_FUNCTION
%token TOK_PARAM
%token TOK_CALL
%token TOK_BLOCK 
%token TOK_RETURN

%right TOK_IF TOK_ELSE
%right '='
%left  TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%left  '+' '-'
%left  '*' '/' '%'
%right TOK_POS TOK_NEG '!' TOK_NEW
%left  TOK_ARRAY TOK_FIELD TOK_FUNCTION
%left  '[' '.'

%nonassoc '('

%start start

%%

start: program                  { $$ = $1 = nullptr; }
        ;

program: program structdef      { $$ = $1->adopt ($2); }
        | program function      { $$ = $1->adopt ($2); }
        | program globaldecl    { $$ = $1->adopt ($2); }
        | program error '}'     { $$ = $1; destroy($3); }
        | program error ';'     { $$ = $1; destroy($3); }
        |                       { $$ = parser::root; }
        ;

structdef: TOK_STRUCT TOK_IDENT '{' '}'
                        {
                                destroy ($3, $4);
                                $$ = $1->adopt($2->sym(TOK_TYPEID));
                        }
        | TOK_STRUCT TOK_IDENT fieldrecur '}'
                        {
                                destroy ($4);
                                $$ = $1->adopt($2->sym(TOK_TYPEID)
                                        , $3->sym(TOK_FIELD));
                        }
        ;

fielddecl: basetype TOK_ARRAY TOK_IDENT ';'
                        { 
                                destroy($4);
                                $$ = $2->adopt($1, $3);
                        }
        | basetype TOK_IDENT ';' 
                        { 
                                destroy($3);
                                $$ = $1->adopt($2);
                        }
        ;

fieldrecur: '{' fielddecl            
                        { 
                                $$ = $1->adopt($2);
                        }
        | fieldrecur fielddecl   
                        { 
                                $$ = $1->adopt($2);
                        }
        ;

basetype: TOK_VOID              { $$ = $1; }
        | TOK_INT               { $$ = $1; }
        | TOK_STRING            { $$ = $1; }
        | TOK_IDENT             { $$ = $1->sym(TOK_TYPEID); }
        ;

globaldecl: identdecl '=' constant ';'
                        { 
                                destroy ($4);
                                $2->sym(TOK_VARDECL);
                                $$ = $2->adopt ($1, $3); 
                        }
        ;

function: identdecl '(' ')' fnbody
                        {       
                                destroy ($3);
                                $2->sym(TOK_PARAM);
                                $$ = new astree(TOK_FUNCTION
                                        , $1->lloc, "");
                                $$ = $$->adopt ($1, $2, $4); 
                        }
        | identdecl arguments ')' fnbody
                        {       
                                destroy ($3);
                                $$ = new astree(TOK_FUNCTION
                                        , $1->lloc, "");
                                $$ = $$->adopt ($1, $2, $4); 
                        }
        | identdecl '(' ')' ';'
                        {       
                                destroy ($3, $4);
                                $2->sym(TOK_PARAM);
                                $$ = new astree(TOK_PROTO
                                        , $1->lloc, "");
                                $$ = $$->adopt ($1, $2); 
                        }
        | identdecl arguments ')' ';'
                        {       
                                destroy ($3, $4);
                                $$ = new astree(TOK_PROTO
                                        , $1->lloc, "");
                                $$ = $$->adopt ($1, $2);
                        }
        ;

arguments: '(' identdecl
                        {       
                                $1->sym(TOK_PARAM);
                                $$ = $1->adopt ($2); 
                        }
        | arguments ',' identdecl
                        {     
                                destroy ($2);  
                                $$ = $1->adopt ($3);  
                        }
        ;

identdecl: basetype TOK_ARRAY TOK_IDENT 
                        { 
                                $$ = $2->adopt($1
                                , $3->sym(TOK_DECLID));
                        }
        | basetype TOK_IDENT  
                        { 
                                $$ = $1->adopt($2->sym(TOK_DECLID));
                        }
        ;

fnbody:  fnbdrecur '}'
                        { 
                                destroy ($2);
                                $$ = $1;
                        }
        | '{' '}'
                        { 
                                destroy ($2);
                                $1->sym(TOK_BLOCK);
                                $$ = $1;
                        }
        ;

fnbdrecur: '{' localdecl        
                        { 
                                $1->sym(TOK_BLOCK); 
                                $$ = $1->adopt ($2); 
                        }
        | '{' statement         
                        { 
                                $1->sym(TOK_BLOCK); 
                                $$ = $1->adopt ($2); 
                        }
        | fnbdrecur localdecl   { $$ = $1->adopt ($2); }
        | fnbdrecur statement   { $$ = $1->adopt ($2); }
        ;

localdecl: identdecl '=' expr ';'
                        { 
                                destroy ($4);
                                $2->sym(TOK_VARDECL);
                                $$ = $2->adopt ($1, $3); 
                        }
        ;

statement: block                { $$ = $1; }
        | while                 { $$ = $1; }
        | ifelse                { $$ = $1; }
        | return                { $$ = $1; }   
        | ';'                  
                        { 
                                destroy ($1); 
                                $$ = nullptr;
                        }
        | expr ';'      {
                                destroy ($2);
                                $$ = $1;
                        }
        ;

block: '{' blockrecur '}'
                        {
                                destroy ($3);
                                $1->sym(TOK_BLOCK);
                                $$ = $1->adopt ($2);
                        }
        ;

blockrecur: statement           { $$ = $1; }
        | blockrecur statement  { $$ = $1->adopt ($2); }
        ;

while: TOK_WHILE '(' expr ')' statement
                        { 
                                destroy ($2, $4);
                                $$ = $1->adopt ($3, $5);
                        }
        ;

ifelse: TOK_IF '(' expr ')' statement TOK_ELSE statement
                        { 
                                destroy ($2, $4, $6);
                                $$ = $1->adopt ($3, $5, $7);
                        }
        | TOK_IF '(' expr ')' statement %prec TOK_ELSE
                        { 
                                destroy ($2, $4);
                                $$ = $1->adopt ($3, $5);
                        }
        ;

return: TOK_RETURN ';'        
                        {
                                destroy ($2);
                                $$ = $1;
                        }
        | TOK_RETURN expr ';'   
                        { 
                                destroy ($3);
                                $$ = $1->adopt ($2);
                        }
        ;

expr: expr binop expr           { $$ = $2->adopt ($1, $3); }
        | unop expr             { $$ = $1->adopt ($2); }
        | allocation            { $$ = $1; }
        | call                  { $$ = $1; }
        | variable              { $$ = $1; }
        | constant              { $$ = $1; }
        | '(' expr ')'          
                {
                                destroy ($1, $3);
                                $$ = $2;                       
                }
        ;

allocation: TOK_NEW TOK_IDENT { $$ = $1->adopt($2->sym(TOK_TYPEID)); }
        | TOK_NEW TOK_STRING '(' expr ')'
                        {
                                destroy ($2, $3, $5);
                                $1->sym(TOK_NEWSTR);
                                $$ = $1->adopt ($4);    
                        }
        | TOK_NEW basetype '[' expr ']'
                        {
                                destroy ($3, $5);
                                $1->sym(TOK_NEWARRAY);
                                $$ = $1->adopt ($2, $4);   
                        }
        ;

call: TOK_IDENT '(' ')'
                        { 
                                destroy ($3);
                                $2->sym(TOK_CALL);
                                $$ = $2->adopt($1);
                        }
        | TOK_IDENT '(' callargs ')'
                        { 
                                destroy ($4);
                                $2->sym(TOK_CALL);
                                $$ = $2->adopt ($1, $3); 
                        }
        ;

callargs: expr          { $$ = $1; }                 
        | callargs ',' expr
                        { 
                                destroy ($2);
                                $$ = $1->adopt ($3); 
                        }
        ;

variable: TOK_IDENT             { $$ = $1; }
        | expr '[' expr ']'     
                        { 
                                destroy ($4);
                                $$ = $2->adopt($1, $3);
                        }
        | expr '.' TOK_FIELD    
                        {
                                $$ = $2->adopt($1, $3);
                        }
        ;

constant: TOK_INTCON            { $$ = $1; }
        | TOK_CHARCON           { $$ = $1; }
        | TOK_STRINGCON         { $$ = $1; }
        | TOK_NULL              { $$ = $1; }
        ;

binop:    TOK_EQ                { $$ = $1; }
        | TOK_NE                { $$ = $1; }
        | TOK_LT                { $$ = $1; }
        | TOK_LE                { $$ = $1; }
        | TOK_GT                { $$ = $1; }
        | TOK_GE                { $$ = $1; }
        | '+'                   { $$ = $1; }
        | '-'                   { $$ = $1; }
        | '*'                   { $$ = $1; }
        | '/'                   { $$ = $1; }
        | '='                   { $$ = $1; }
        ;

unop:     '+'                   { $$ = $1->sym(TOK_POS); }
        | '-'                   { $$ = $1->sym(TOK_NEG); }
        | '!'                   { $$ = $1->sym(TOK_NOT); }
        | TOK_NOT               { $$ = $1; }
        ;

%%

const char* parser::get_tname (int _symbol) {
   return yytname [YYTRANSLATE (_symbol)];
}

