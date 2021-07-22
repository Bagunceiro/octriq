#pragma once

const struct OpcodeDef_struct
{
    const char* mnemonic;
    const int   code;
    const int   numops;  // Number of operands except 3 which indicates it has 1 op but in pos 2
} opcodedef[] = {
{ "nop",  0, 0 },
{ "set",  1, 2 },
{ "clr",  2, 0 },
{ "add",  3, 2 },
{ "sub",  4, 2 },
{ "jmp",  5, 3 },
{ "jsr",  6, 3 },
{ "jnz",  7, 3 },
{ "jz",   8, 3 },
{ "jnv",  9, 3 },
{ "jv",  10, 3 },
{ "ret", 11, 0 },
{ "hlt", 12, 0 },
{ "psh", 13, 1 },
{ "pop", 14, 1 },
{ "run", 15, 3 },
{ (const char*)0,  0 }};

#define BYTE_BITS (8)

#define WIDTH_TC  (10)
#define WIDTH_OPC  (6)
#define WIDTH_OP1  (5)
#define WIDTH_IND  (1)
#define WIDTH_OP2 (10)

#define MOD_TC  (1 << WIDTH_TC)      // 1024 = modulus of the timecode field
#define MOD_OPC (1 << WIDTH_OPC)     // 64   = modulus of the opcode field
#define MOD_OP1 (1 << WIDTH_OP1)     // 32   = modulus of the op1 field
#define MOD_IND (1 << WIDTH_IND)     // 2    = modulus of the op1 indirection field
#define MOD_OP2 (1 << WIDTH_OP2)     // 1024 = modulus of the op2 field

#define NCHANNELS   MOD_OP1/2        // 16                          number of channels allowed by the language
#define NREGISTERS  MOD_OP1/2        // 16                          number of registers allowed by the language
#define REG_MASK    (NCHANNELS -1)   // 16-1        = 15 =   1111   bits holding the register number
#define REGIND_MASK (NCHANNELS)      // 16               =  10000   bit indicating that the op is a register
#define OPC_MASK    ((MOD_OPC/2) -1) // (2^6 /2) -1 = 31 =  11111   bits holding the opcode
#define IND_MASK    ((MOD_OPC/2))    // (2^6)       = 64 = 100000   bit indicates rval is indirect

#define OFFSET_TC  (0)
#define OFFSET_OPC (WIDTH_TC)
#define OFFSET_OP1 (OFFSET_OPC + WIDTH_OPC)
#define OFFSET_IND (OFFSET_OP1 + WIDTH_OP1)
#define OFFSET_OP2 (OFFSET_IND + WIDTH_IND)

#define INSTR_BITS (OFFSET_OP2 + WIDTH_OP2)
#define INSTR_BYTES (INSTR_BITS/8)

typedef struct OpcodeDef_struct OpcodeDef;

int findOpcode(char* mnemonic);
