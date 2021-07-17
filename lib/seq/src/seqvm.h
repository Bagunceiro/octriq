#pragma once

#include <map>

#ifdef ARDUINO
#include <LITTLEFS.h>
#else
#define File FILE *
#endif

#include "seqlang.h"

#ifdef ARDUINO
#define LOGF(...) Serial.printf(__VA_ARGS__)
#define OPENFILE(fname, mode) LITTLEFS.open(fname, mode)
#define CLOSEFILE(file) file.close()
#define READFILE(file, buffer, size) file.read((uint8_t *)buffer, size)
#define SEEKFILE(file, pos, mode) file.seek(pos, mode)
#define POSINFILE(file) file.position()
#define SEEKMODE_SET SeekSet
#define SEEKMODE_END SeekEnd
#else
#define File FILE *
#define OPENFILE(fname, mode) fopen(fname, mode)
#define CLOSEFILE(file) fclose(file)
#define READFILE(file, buffer, size) fread((void*)buffer, size, 1, file)
#define SEEKFILE(file, pos, mode) fseek(file, pos, mode)
#define POSINFILE(file) ftell(file)
#define SEEKMODE_SET SEEK_SET
#define SEEKMODE_END SEEK_END
#define LOGF(...) printf(__VA_ARGS__)
#endif

class Text
{
    public:
    Text();
    virtual ~Text();
    uint32_t operator[](int index) { return memory[index]; }
    unsigned int load(File f);
    size_t getSize() { return size; }
    private:
    size_t size;
    uint32_t* memory;
};

class VM
{
public:
    VM(File f, int stk = 16);
    virtual ~VM();
    void exec();
    static void buildOpMap();
    void dumpRegisters()
    {
        for (int i = 0; i < NREGISTERS; i++)
            printf("Reg %x = %d\n", i, reg[i]);
    }
    void setStack(int size);
    void start(int address = 0);
    void startAsTask(int address, int stacksize = 4096);
    void settrace(bool t) { trace = t; }
private:
#ifdef ARDUINO
    void createTask(const char* n, int stack);
    TaskHandle_t xHandle;
    static void doExec(void*);
#endif

    Text txt;
    int reg[NREGISTERS];
    int stackSize;
    int *stack;
    uint8_t stackptr;
    int progCounter;
    void push(int val);
    int pop();
    bool fetch(unsigned long *instr);
    void jumpto(int address);
//   File binfile;
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
    static std::map<int, int (VM::*)(int, int)> opmap;
    bool zero;
    bool trace;
    static int numVMs;
    bool abort;
};
