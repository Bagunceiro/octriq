%{
#include <stdio.h>
#include <string.h>
#include "seqasm.h"
%}

%token ID
%token INT
%token EOL
%token CHANNEL
%token REGISTER

%union
{
    int intVal;
    char *strVal;
}

/*
*/
%%

prog:                     { /* printf("prog: (empty)\n"); */ }
| prog line               { /* printf("prog: prog line\n"); */}
;

line: identifier ':'      { constructLabel(); }
 | identifier '=' integer { constructAssignment(); }
 | instruction            { constructInstruction(); }
 | error EOL              { nextLine(); }
 | line EOL               { nextLine(); }
;

instruction:
opcode ';'                { }
 | opcode operands  ';'   { }
;

operands:
lvalue ',' operand      { /* printf("operands: lvalue, operand\n"); */ }
| operand                   { /* printf("operands: operand\n"); */ }
;

operand: value
| lvalue
;

lvalue:
register
| channel
| REGISTER '(' register ')'	{ isRegIndirect(); }
| CHANNEL '(' register ')'	{ isChanIndirect(); }
;

channel: CHANNEL value       { isChannel(); }
;

register: REGISTER value       { isRegister(); }
;

opcode: identifier        { isOpcode(); }
;

value: integer            { /* printf("value: integer\n"); */ }
 | identifier             { /* printf("value: identifier\n"); */ }
;

integer: INT              { makeIntNode(yylval.intVal); }
;
identifier: ID            { makeIdNode(yylval.strVal); }
;
