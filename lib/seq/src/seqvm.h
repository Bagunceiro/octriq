#pragma once

#include <map>

#ifdef ARDUINO
#include <LITTLEFS.h>
#else
#define File FILE*
#endif

#include "seqlang.h"

class VM
{
public:
    VM(const char* filename, int stk = 16);
    virtual ~VM();
    void exec(int address = 0);
    static void buildOpMap();
    void dumpRegisters()
    {
        for (int i = 0; i < NREGISTERS; i++) printf("Reg %x = %d\n", i, reg[i]);
    }
    void setStack(int size);
private:
    int reg[NREGISTERS];
    int stackSize;
    int *stack;
    uint8_t stackptr;
    int progCounter;
    void push(int val);
    int pop();
    bool fetch(unsigned long* instr);
    void jumpto(int address);
    File binfile;
    int func_nop(int, int);
    int func_set(int, int);
    int func_clr(int, int);
    int func_add(int, int);
    int func_sub(int, int);
    int func_jmp(int, int);
    int func_jsr(int, int);
    int func_jnz(int, int);
    int func_ret(int, int);
    int func_hlt(int, int);
    int func_psh(int, int);
    int func_pop(int, int);
    int func_run(int, int);
    static std::map<int, int(VM::*)(int,int)> opmap;
    bool zero;
    bool trace;
};
