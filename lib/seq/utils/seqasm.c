#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>

#include "seq.tab.h"
#include "seqlang.h"
#include "seqasm.h"

extern FILE *yyin;

int line = 0;
int address = 0;
int errors = 0;

#define MAX_SYMBOLS (64)
#define MAX_FWDS (64)

FILE *binfile;

struct Node_struct
{
    enum
    {
        NODE_INT,
        NODE_ID,
    } datatype;
    enum
    {
        NODE_VAL,
        NODE_OPCODE,
        NODE_CHANNEL,
        NODE_REGISTER,
        NODE_INDIRECT,
        NODE_TIMECODE
    } nodetype;
    union
    {
        int intval;
        char *strval;
    } val;
} *node = null;

struct Symbol_struct
{
    char *name;
    int val;
} symbol[MAX_SYMBOLS];
int nsymbols = 0;

struct FwdRef_struct
{
    char *name;
    int add;
    int field;
} fwd[MAX_FWDS];
int nfwd = 0;

typedef struct FwdRef_struct FwdRef;
typedef struct Node_struct Node;
typedef struct Symbol_struct Symbol;

Node *parsedLine[8];
int parsepos = 0;

int findOpcode(char *mnemonic)
{
    int result = -1;
    for (int i = 0; opcodedef[i].mnemonic; i++)
    {
        if (strcasecmp(opcodedef[i].mnemonic, mnemonic) == 0)
        {
            result = opcodedef[i].code;
            break;
        }
    }
    return result;
}

int yyerror(const char *msg)
{
    fprintf(stderr, "Line %d: %s\n", line, msg);
    errors++;
    return 0;
}

int findSymbol(char *name)
{
    int val = -1;
    for (int i = 0; i < nsymbols; i++)
    {
        if (strcmp(symbol[i].name, name) == 0)
        {
            val = symbol[i].val;
            break;
        }
    }
    return val;
}

void addFwd(char *name, int add, int field)
{
    fwd[nfwd].name = strdup(name);
    fwd[nfwd].add = add;
    fwd[nfwd].field = field;
    nfwd++;
}

char *nodeval(Node *n)
{
    char *val = n->val.strval;
    if (n->datatype == NODE_INT)
    {
        static char buffer[16];
        snprintf(buffer, sizeof(buffer) - 1, "%d", n->val.intval);
        val = buffer;
    }
    return val;
}

Node *makeNode()
{
    Node *n = (Node *)malloc(sizeof(Node));
    parsedLine[parsepos] = n;
    parsepos++;
    return n;
}

void makeIntNode(int value)
{
    // printf("Making int %d\n", value);
    node = makeNode();
    node->nodetype = NODE_VAL;
    node->datatype = NODE_INT;
    node->val.intval = value;
}

void makeIdNode(char *value)
{
    // printf("Making id %s\n", value);
    node = makeNode();
    node->nodetype = NODE_VAL;
    node->datatype = NODE_ID;
    node->val.strval = strdup(value);
}

void isOpcode()
{
    node->nodetype = NODE_OPCODE;
    // printf("  Is opcode: %s\n", nodeval(node));

    int opcode = findOpcode(node->val.strval);
    if (opcode < 0)
        yyerror("Unknown opcode");
    free(node->val.strval);
    node->datatype = NODE_INT;
    node->val.intval = opcode;
}

void isRegister()
{
    // if ((node->val.intval & REG_MASK) >= NREGISTERS)
    if ((node->datatype == NODE_INT) && ((node->val.intval) >= NREGISTERS))
        {
            char buffer[48];
            snprintf(buffer, sizeof(buffer) - 1, "Register %x out of range (0-%d)", node->val.intval, NREGISTERS - 1);
            yyerror(buffer);
        }
    node->nodetype = NODE_REGISTER;
    // printf("  Is register: %s\n", nodeval(node));
}

void isIndirect()
{
    node->nodetype = NODE_INDIRECT;
    // printf("  Is indirect: %s\n", nodeval(node));
}

void isChannel()
{
    if ((node->datatype == NODE_INT) && (node->val.intval >= NCHANNELS))
    {
        char buffer[48];
        snprintf(buffer, sizeof(buffer) - 1, "Channel out of range (0-%d)", NCHANNELS - 1);
        yyerror(buffer);
    }
    node->nodetype = NODE_CHANNEL;
    // printf("  Is channel: %s\n", nodeval(node));
}

void isTimecode()
{
    if ((node->datatype == NODE_INT) && (node->val.intval >= MOD_TC))
    {
        char buffer[48];
        snprintf(buffer, sizeof(buffer) - 1, "Timecode out of range (0-%d)", MOD_TC - 1);
        yyerror(buffer);
    }
    node->nodetype = NODE_TIMECODE;
    // printf("  Is timecode: %s\n", nodeval(node));
}

void isComment()
{
    printf("A comment: %s\n", yylval.strVal);
}

void nextLine()
{
    for (int i = 0; i < parsepos; i++)
    {
        free(parsedLine[i]);
    }
    parsepos = 0;

    line++;
    // printf("Line %d\n", line);
}

void createSymbol(char *name, int val)
{
    symbol[nsymbols].name = strdup(name);
    symbol[nsymbols].val = val;
    nsymbols++;
}

void constructLabel()
{
    createSymbol(parsedLine[0]->val.strval, address);
    // printf("LABEL\n");
}

void constructAssignment()
{
    createSymbol(parsedLine[0]->val.strval, parsedLine[1]->val.intval);
    // printf("ASSIGNMENT\n");
}

void outBin(unsigned int val, int bits)
{
    unsigned long mask = 1;
    mask <<= bits - 1;
    int bit = 0;
    while (mask)
    {
        putchar((val & mask) ? '1' : '0');
        mask >>= 1;
        bit++;
        switch (bit)
        {
        case WIDTH_TC:                                     // 10:
        case WIDTH_TC + WIDTH_OPC:                         // 16:
        case WIDTH_TC + WIDTH_OPC + WIDTH_OP1:             // 21:
        case WIDTH_TC + WIDTH_OPC + WIDTH_OP1 + WIDTH_IND: // 22:
            putchar('-');
        }
    }
    putchar('\n');
}

void setField(unsigned int *code, int width, int val, int pos, int bits)
{
    int offset = (sizeof(code) * BYTE_BITS) - width; // bit pos in code that corresponds to pos 0
    int mask = 0;
    for (int i = 0; i < bits; i++)
    {
        mask <<= 1;
        mask |= 1;
    }
    val &= mask;
    mask <<= width - (pos + bits);
    mask = ~mask;
    val <<= width - (pos + bits);

    *code &= mask;
    *code |= val;
}

void constructInstruction()
{
    // printf("INSTRUCTION %d:", address);
    int timecode = 0;
    int opcode = 0;
    int opcount = 0;
    int lval = 0;
    int rval = 0;
    int rvaltype = 0;

    int i;

    /*
    for (i = 0; i < parsepos; i++)
    {
        printf(" %s", nodeval(parsedLine[i]));
    }
    printf("\n");
*/

    int gotlval = 0;
    for (int count = 0; count < parsepos; count++)
    {
        int field = -1;
        int val = -1; // signals forward reference
        if (parsedLine[count]->datatype == NODE_INT)
        {
            val = parsedLine[count]->val.intval;
        }
        else
        {
            val = findSymbol(parsedLine[count]->val.strval);
        }
        switch (parsedLine[count]->nodetype)
        {
        case NODE_TIMECODE:
            field = 0;
            timecode = val;
            break;
        case NODE_OPCODE:
            field = 1;
            opcode = val;
            break;
        case NODE_REGISTER:
            val |= REGIND_MASK;
        case NODE_CHANNEL:
            if (gotlval)
            {
                rval = val;
                rvaltype = 1;
                field = 4;
            }
            else
            {
                lval = val;
                field = 2;
                gotlval = 1;
            }
            break;
        case NODE_INDIRECT:
            field = 2;
            val |= REGIND_MASK;
            lval = val;
            opcode |= IND_MASK;
            break;
        case NODE_VAL:
            if ((node->datatype == NODE_INT) && (node->val.intval >= MOD_OP2))
            {
                char buffer[48];
                snprintf(buffer, sizeof(buffer) - 1, "Value out of range (0-%d)", MOD_OP2 - 1);
                yyerror(buffer);
            }
            else
            {
                field = 4;
                rval = val;
            }
        }
        if (val < 0)
        {
            addFwd(parsedLine[count]->val.strval, address, field);
        }
    }

    // printf ("CODE: %02x %02x %02x %1x %04x\n", timecode, opcode, lval, rvaltype, rval);
    unsigned int code;
    setField(&code, INSTR_BITS, timecode, OFFSET_TC, WIDTH_TC);
    setField(&code, INSTR_BITS, opcode, OFFSET_OPC, WIDTH_OPC);
    setField(&code, INSTR_BITS, lval, OFFSET_OP1, WIDTH_OP1);
    setField(&code, INSTR_BITS, rvaltype, OFFSET_IND, WIDTH_IND);
    setField(&code, INSTR_BITS, rval, OFFSET_OP2, WIDTH_OP2);
    fwrite(&code, INSTR_BITS / BYTE_BITS, 1, binfile);
    // outBin(code, INSTR_BITS);

    address++;
}

void dumpSymbolTable()
{
    for (int i = 0; i < nsymbols; i++)
    {
        printf("%s\t%d\n", symbol[i].name, symbol[i].val);
    }
}

void dumpFwds()
{
    for (int i = 0; i < nfwd; i++)
    {
        printf("%s\t%d\t%d\n", fwd[i].name, fwd[i].add, fwd[i].field);
    }
}

void resolveFwd()
{
    for (int i = 0; i < nfwd; i++)
    {
        int pos = -1, bits = -1;
        switch (fwd[i].field)
        {
        case 0:
            pos = OFFSET_TC, bits = WIDTH_TC;
            break;
        case 1:
            pos = OFFSET_OPC, bits = WIDTH_OPC;
            break;
        case 2:
            pos = OFFSET_OP1, bits = WIDTH_OP1;
            break;
        case 3:
            pos = OFFSET_IND, bits = WIDTH_IND;
            break;
        case 4:
            pos = OFFSET_OP2, bits = WIDTH_OP2;
            break;
        }
        int code;
        int val = findSymbol(fwd[i].name);
        fseek(binfile, (fwd[i].add) * INSTR_BYTES, SEEK_SET);
        fread(&code, INSTR_BYTES, 1, binfile);
        if (pos >= 0)
            setField(&code, INSTR_BITS, val, pos, bits);
        fseek(binfile, -INSTR_BYTES, SEEK_CUR);
        fwrite(&code, INSTR_BYTES, 1, binfile);
    }
}

void dumpBinary()
{
    int add = 0;
    int code;
    FILE *fp = fopen("seq.bin", "r");
    while (fread(&code, INSTR_BYTES, 1, binfile) > 0)
    {
        printf("%04x ", add++);
        outBin(code, INSTR_BITS);
    }
    fclose(fp);
}

int main(int argc, char *argv[])
{
    int opt;
    int exitcode = 0;
    char *binary = NULL;
    char *input = NULL;
    FILE *outfile = stdout;

    while ((opt = getopt(argc, argv, "i:o:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            input = optarg;
            break;
        case 'o':
            binary = optarg;
            break;
        default:
            exitcode = -1;
            break;
        }
    }
    if (optind < argc)
    {
        input = argv[optind];
    }

    if (binary)
    {
        outfile = fopen(binary, "w");
        if (!outfile)
        {
            perror("Could not open output");
            exitcode = -2;
        }
    }

    if (input)
    {
        yyin = fopen(input, "r");
        if (!yyin)
        {
            perror("Could not open input");
            exitcode = -2;
        }
    }

    if (isatty(fileno(outfile)))
    {
        fprintf(stderr, "Cannot write binary to a terminal\n");
        exitcode = -3;
    }

    if (exitcode == 0)
    {
        char *tmpfilename = strdup("/tmp/seq.bin.XXXXXX");
        int tmpfd = mkstemp(tmpfilename);
        binfile = fdopen(tmpfd, "w+");
        if (binfile)
        {
            address = 0;
            nextLine();
            yyparse();
            resolveFwd();
            if (errors)
            {
                fprintf(stderr, "Errors: %d\n", errors);
            }
            else
            {
                fseek(binfile, 0, SEEK_SET);
                unsigned char c;
                while (fread(&c, 1, 1, binfile))
                {
                    fwrite(&c, 1, 1, outfile);
                }
                fprintf(stderr, "Processed %d source -> %d binary\n", line - 1, address);
            }
            fclose(binfile);
            unlink(tmpfilename);
            if (binary)
                fclose(outfile); // ie if it's not stdout
        }
        else
        {
            perror("Could not open temporary file");
        }
    }
    return exitcode;
}
