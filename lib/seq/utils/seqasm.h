enum fieldcode {
    F_OPCODE,
    F_OP1,
    F_OP2,
    F_OP3       // combined op1 and op3
};

#define null ((void*)0)

int yylex();
int yyerror(const char *);
void addLabel(char* l);
void addSymbol(char* l, int v);
int getLabelValue(char* l, enum fieldcode f);
int addOp(int opcode, int operand1, int operand2);

void makeIntNode(int value);
void makeIdNode(char* value);
void isOpcode();
void isChannel();
void isRegister();
void isOperandVal();
void isComment();
void isIndirect();
void isRegIndirect();
void isChanIndirect();

void constructLabel();
void constructAssignment();
void constructInstruction();
void nextLine();

extern int line;
