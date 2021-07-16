/* recognize tokens for the calculator and print them out */
%{
# include "seq.tab.h"
extern int line;
%}

comment     "//".*
mlcomment "/\*"([^\*]|\*[^/])*"\*/"

/*
*/

%%
{mlcomment} { for (int i = 0; i < strlen(yytext); i++) if (yytext[i] == '\n') line++; }
{comment}
[0-9]+ { /* printf("INT\n"); */ yylval.intVal = atoi(yytext); return INT; }
[A-Za-z][A-Za-z0-9]* { /* printf("ID\n");*/ yylval.strVal = strdup(yytext); return ID; }
\n      { return EOL; }
[ \t]	{ }
"%"	{ return(REGISTER); }
"#"	{ return(CHANNEL); }
.       { return yytext[0]; }

%%
