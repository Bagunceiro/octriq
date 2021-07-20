#pragma once

#include <map>
#include <memory>

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
#define READFILE(file, buffer, size) fread((void *)buffer, size, 1, file)
#define SEEKFILE(file, pos, mode) fseek(file, pos, mode)
#define POSINFILE(file) ftell(file)
#define SEEKMODE_SET SEEK_SET
#define SEEKMODE_END SEEK_END
#define LOGF(...) printf(__VA_ARGS__)
#endif

struct Memory
{
    uint32_t *int32s;
};

class Text
{
public:
    Text();
    Text(const Text &rhs);
    Text &operator=(const Text &rhs);
    virtual ~Text();
    // uint32_t getInt(int index);
    unsigned int load(File f);
    size_t getSize() { return size; }
    uint32_t operator[](int i) { return (memory.get)()[i]; }
    int memrefs() { return memory.use_count(); }

private:
    size_t size;
    // uint32_t* memory;
    std::shared_ptr<uint32_t> memory;
};

class VM
{
public:
    VM(File f, int stk = 16);
    VM(const VM &rhs);
    VM &operator=(const VM &rhs);
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
    void startAsTask(int address, int stacksize = 3072);
    void settrace(bool t) { trace = t; }
    //   int memrefs() { return txt.memrefs(); }
    static int kill(int);
    int getNumber() { return vmnumber; }
#ifdef ARDUINO
    static int listTasks(Print &out);
#endif
private:
#ifdef ARDUINO
    void createTask(const char *n, int stack);
    TaskHandle_t xHandle;
    static void doExec(void *);
#ifdef TUNE_STACK_SIZE
    int minhwm;
#endif TUNE_STACK_SIZE
#endif

    Text txt;
    int reg[NREGISTERS];
    int stackSize;
    int *stack;
    uint8_t stackptr;
    unsigned int progCounter;
    int vmnumber;
    void push(unsigned int val);
    unsigned int pop();
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
    int func_jz(int, int);
    int func_jnc(int, int);
    int func_jc(int, int);
    int func_ret(int, int);
    int func_hlt(int, int);
    int func_psh(int, int);
    int func_pop(int, int);
    int func_run(int, int);
    static std::map<int, int (VM::*)(int, int)> opmap;
    bool zero;
    bool carry;
    bool trace;
    static int numVMs;
    // bool abort;
    static std::map<int, VM *> tasklist;
    bool halt;
};
