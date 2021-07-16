#include <stdio.h>
#include <string.h>

#include "seqlang.h"

int readInstruction(FILE* infile, unsigned long *instr)
{
   return(fread(instr, INSTR_BYTES, 1, infile));
}

unsigned long getField(unsigned long code, int width, int pos, int bits)
{
    int offset = (sizeof(code) * BYTE_BITS) - width; // bit pos in code that corresponds to pos 0
    int mask = 0;
    for (int i = 0; i < bits; i++)
    {
        mask <<=1;
        mask |=1;
    }
    int mask2 = mask << (width - (pos + bits));
    unsigned int val = code & mask2;
    val >>= width - (pos + bits);
    return val;
}

const char* findMnemonic(unsigned long opcode, int* numops)
{
    for (int i = 0; opcodedef[i].mnemonic; i++)
    if (opcodedef[i].code == opcode)
    {
        *numops = opcodedef[i].numops;
        return opcodedef[i].mnemonic;
    }
    *numops = 0;
    return "???";
}

int main(int argc, char* argv[])
{
    unsigned address = 0;
    FILE* infile = stdin;
    unsigned long instr;
    while (readInstruction(infile, &instr) >0)
    {
        char tcstr[6];
        int numops;
        unsigned long tc = getField(instr, INSTR_BITS, OFFSET_TC, WIDTH_TC);
        tcstr[0] = '\00';
        if (tc > 0) sprintf(tcstr, "%4ld ", tc);
        unsigned long opcode = getField(instr, INSTR_BITS, OFFSET_OPC, WIDTH_OPC);
        unsigned long indirect = (opcode & IND_MASK);
        const char* mnemonic = findMnemonic(opcode & OPC_MASK, &numops);
        int op1raw = getField(instr, INSTR_BITS, OFFSET_OP1, WIDTH_OP1);
        int op1 = op1raw;
        char* pfx1 = "#";
        if (op1 >= NCHANNELS)
        {
            pfx1 = "%";
            op1 &= REG_MASK;
        }

        int ind = getField(instr, INSTR_BITS, OFFSET_IND, WIDTH_IND);
        int op2raw = getField(instr, INSTR_BITS, OFFSET_OP2, WIDTH_OP2);
        int op2 = op2raw;
        char* pfx2 = "";
        if (ind)
        {
            if (op2 >= NCHANNELS)
            {
                pfx2 = "%";
                op2 &= REG_MASK;
            }
            else pfx2 = "#";
        }

        printf("%04x %03lx %02lx %02x %1x %03x", address, tc, opcode, op1raw, ind, op2raw);
        printf("  %6s ", tcstr);
        printf("%4s", mnemonic);
        char* openind = (indirect ? "(" : "");
        char* closeind = (indirect ? ")" : "");
        switch(numops)
        {
            case 1:
                printf(" %s%s%d%s", openind, pfx1, op1, closeind);
                break;
            case 2:
                printf(" %s%s%d%s,%s%d", openind, pfx1, op1, closeind, pfx2, op2);
                break;
            case 3:
                printf(" %s%d", pfx2, op2);
                break;
        }
        printf(";\n");
        address++;
    }
    return 0;
}
