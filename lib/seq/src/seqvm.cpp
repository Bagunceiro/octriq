#ifdef ARDUINO
#include <LITTLEFS.h>
#define LOG(...) Serial.printf(__VA_ARGS__)
#define OPENFILE(fname,mode) LITTLEFS.open(fname, mode)
#define CLOSEFILE(file) file.close()
#define READFILE(file, buffer, size) file.read((uint8_t*)buffer, size) 
#define SEEKFILE(file, pos, mode) file.seek(pos,mode);
#define SEEKMODE_SET SeekSet
#else
#define File FILE*
#define OPENFILE(fname,mode) fopen(fname, mode);
#define CLOSEFILE(file) fclose(file)
#define READFILE(file, buffer, size) fread(buffer, size, 1, file)
#define SEEKFILE(file, pos, mode) fseek(file, pos, mode); 
#define SEEKMODE_SET SEEK_SET
#define LOG(...) printf(__VA_ARGS__)

unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec /1000);
}

void delay(unsigned int msecs)
{
    unsigned int usecs = msecs * 1000;
    fflush(stdout);
    usleep(usecs);
}

#endif

#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include "seqlang.h"
#include "seqvm.h"



class Instruction
{
public:
    Instruction(const unsigned long ins);
    virtual ~Instruction();
    int timecode() { return _timecode; }
    int opcode() { return _opcode; }
    int op1() { return _op1; }
    int ind() { return _ind; }
    int op2() { return _op2; }
    void dump();
    
private:
    void deconstruct(const unsigned long ins);
    unsigned int getField(unsigned long code, int width, int pos, int bits);
    unsigned int _timecode;
    unsigned int _opcode;
    unsigned int _op1;
    unsigned int _ind;
    unsigned int _op2;
};

void Instruction::dump()
{
    printf("%04x %02x %02x %01x %02x\n", _timecode, _opcode, _op1, _ind, _op2);
}

Instruction::Instruction(const unsigned long ins)
{
    deconstruct(ins);
}

Instruction::~Instruction()
{
}

unsigned int Instruction::getField(unsigned long code, int width, int pos, int bits)
{
    unsigned int mask = 0;
    // Make a "bits" bit wide mask of 1s
    for (int i = 0; i < bits; i++)
    {
        mask <<=1;
        mask |=1;
    }
    // Make a mask for the actual field in the code
    unsigned int mask2 = mask << (width - (pos + bits));
    // chop the field out of the code into "val"
    unsigned long val = code & mask2;
    // shift val back down 
    val >>= width - (pos + bits);
    return val;
}

void Instruction::deconstruct(const unsigned long code)
{
    _timecode = getField(code, INSTR_BITS, OFFSET_TC,  WIDTH_TC);
    _opcode   = getField(code, INSTR_BITS, OFFSET_OPC, WIDTH_OPC);
    _op1      = getField(code, INSTR_BITS, OFFSET_OP1, WIDTH_OP1);
    _ind      = getField(code, INSTR_BITS, OFFSET_IND, WIDTH_IND);
    _op2      = getField(code, INSTR_BITS, OFFSET_OP2, WIDTH_OP2);
}

struct Channel
{
public:
    void set(int v) { value = v; }
    int get() { return value; }
    void dump();
private:
    int pin;
    int value;
} chan[NCHANNELS];

void dumpChannels()
{
    for (int i = 0; i < NCHANNELS; i++)
    {
        printf("Channel %x = %d\n", i, chan[i].get());
    }
}

std::map<int, int(VM::*)(int,int)> VM::opmap;
// typdef int(VM::*opfunc)(int,int);

VM::VM(const char* filename, int stk)
{
    binfile = OPENFILE(filename, "r");
    if (!binfile)
    {
        LOG("Could not open %s\n", filename);
    }
    stackptr = 0;
    zero = 0;
    trace = true;
    stack = NULL;
    setStack(stackSize);
}

void VM::setStack(int size)
{
    if (stack) free(stack);
    stack = (int*)malloc(size * sizeof(int));
    stackSize = size;
}

VM::~VM()
{
    if (binfile) CLOSEFILE(binfile);
}

void VM::push(int val)
{
    stack[stackptr] = val;
    stackptr++;
    // Check for overflow
}

int VM::pop()
{
    stackptr--;
    // Check for underflow
    return stack[stackptr];
}

bool VM::fetch(unsigned long* val)
{
    if (READFILE(binfile, val, INSTR_BYTES) > 0)
    {
        progCounter++;
        return true;
    }
    return false;
}

void VM::jumpto(int address)
{
    progCounter = address;
    SEEKFILE(binfile, progCounter * INSTR_BYTES, SEEKMODE_SET);
}


void VM::exec(int address)
{
    unsigned long started = millis();
    unsigned long due = started;
    jumpto(address);
    while (true)
    {
        unsigned long now = millis();
        int tmpadd = progCounter;
        unsigned long insbin;
        if (fetch(&insbin))
        {
            Instruction in(insbin);
            // in.dump();
            unsigned int timecode = in.timecode();
            if (timecode)
            {
                due = due + timecode;
                unsigned int sleeptime=(due - now);
    
                delay(sleeptime);
            }
            int (VM::*func)(int,int) = opmap[in.opcode() & OPC_MASK];
    
            if (trace) printf("%10ld %04x: ", millis() - started, tmpadd);
            if (func)
            {
                int lval = in.op1();
                if (in.opcode() & IND_MASK)
                {
                // lval is indirect (actual lval is held in %lval)
                    lval &= REG_MASK;
                    lval = reg[lval];
                }
                int rval = in.op2();
                if (in.ind())
                {
                    if (rval > NCHANNELS) rval = reg[rval & REG_MASK];
                    else rval = chan[rval].get();
                }
                if ((this->*func)(lval, rval) < 0) break;;
            }
            else
            {
                printf("Unknown opcode %x\n", in.opcode());
                break;
            }
        }
        else break; // fallen off the end;
    }
}

bool isreg(int* index)
{
    int reg = false;
    if (*index > NCHANNELS)
    {
        reg = true;
        *index &= REG_MASK;
    }
    return reg;
}

int VM::func_nop(int, int)
{
    printf("NOP\n");
    return 0;
}

int VM::func_set(int lval, int rval)
{
    bool lvalreg = isreg(&lval);
    if (trace) printf("SET %s%d to %d\n", (lvalreg ? "%" : "#"), lval, rval);
    if (lvalreg)
    {
        reg[lval] = rval;
    }
    else
    {
        chan[lval].set(rval);
    }
    return 0;
}

int VM::func_clr(int, int)
{
    if (trace) printf("CLR\n");
    for (int i = 0; i < NCHANNELS; i++)
    {
        chan[i].set(0);
    }
    return 0;
}

int VM::func_add(int lval, int rval)
{
    bool lvalreg = isreg(&lval);
    if (trace) printf("ADD %d to %s%d ", rval, (lvalreg ? "%" : "#"), lval);
    if (lvalreg)
    {
        reg[lval] = (reg[lval] + rval) % MOD_OP2;
        if (trace) printf("(=%d)\n", reg[lval]);
        zero = (reg[lval] == 0);
    }
    else
    {
        chan[lval].set((chan[lval].get() + rval) % MOD_OP2);
        if (trace) printf("(=%d)\n", chan[lval].get());
        zero = (chan[lval].get() == 0);
    }
    
    return 0;
}

int VM::func_sub(int lval, int rval)
{
    bool lvalreg = isreg(&lval);
    if (trace) printf("SUB %d from %s%d", rval, (lvalreg ? "%" : "#"), lval);
    if (lvalreg)
    {
        reg[lval] = (reg[lval] - rval) % MOD_OP2;
        if (trace) printf("(=%d)\n", reg[lval]);
        zero = (reg[lval] == 0);
    }
    else
    {
        chan[lval].set((chan[lval].get() - rval) % MOD_OP2);
        if (trace) printf("(=%d)\n", chan[lval].get());
        zero = (chan[lval].get() == 0);
    }
    
    return 0;
}

int VM::func_jmp(int, int rval)
{
    jumpto(rval);
    if (trace) printf("JMP to %d\n", rval);
    return 0;
}

int VM::func_jsr(int, int rval)
{
    push(progCounter);
    jumpto(rval);
    if (trace) printf("JSR to %d\n", rval);
    return 0;
}

int VM::func_jnz(int, int rval)
{
    if (!zero)
    {
        jumpto(rval);
        if (trace) printf("JNZ to %d\n", rval);
    }
    else
    {
        if (trace) printf("JNZ ----\n");
    }
    return 0;
}

int VM::func_ret(int, int)
{
    progCounter = pop();
    jumpto(progCounter);
    if (trace) printf("RET (to %d)\n", progCounter);
    return 0;
}

int VM::func_hlt(int, int)
{
    if (trace) printf("HLT\n");
    return -1;
}

int VM::func_psh(int lval, int)
{
    bool lvalreg = isreg(&lval);
    int value;
    if (lvalreg)
    {
        value = reg[lval];
    }
    else
    {
        value = chan[lval].get();
    }
    push(value);
    if (trace) printf("PSH %s%d (%d)\n", (lvalreg ? "%" : "#"), lval, value);
    return 0;
}

int VM::func_pop(int lval, int)
{
    bool lvalreg = isreg(&lval);
    int value = pop();
    if (lvalreg)
    {
        reg[lval] = value;
    }
    else
    {
        chan[lval].set(value);
    }
    if (trace) printf("POP %s%d (%d)\n", (lvalreg ? "%" : "#"), lval, value);
    
    return 0;
}

int VM::func_run(int, int rval)
{
    if (trace) printf("RUN at %d\n", rval);
    return 0;
}

void VM::buildOpMap()
{
    const char* mnemonic;
    for (int i = 0; (mnemonic = opcodedef[i].mnemonic); i++)
    {
        int opcode = opcodedef[i].code;
        if (strcmp(mnemonic, "nop") == 0)
        {
            opmap[opcode] = &VM::func_nop;
        }
        else if (strcmp(mnemonic, "set") == 0)
        {
            opmap[opcode] = &VM::func_set;
        }
        else if (strcmp(mnemonic, "clr") == 0)
        {
            opmap[opcode] = &VM::func_clr;
        }
        else if (strcmp(mnemonic, "add") == 0)
        {
            opmap[opcode] = &VM::func_add;
        }
        else if (strcmp(mnemonic, "sub") == 0)
        {
            opmap[opcode] = &VM::func_sub;
        }
        else if (strcmp(mnemonic, "jmp") == 0)
        {
            opmap[opcode] = &VM::func_jmp;
        }
        else if (strcmp(mnemonic, "jsr") == 0)
        {
            opmap[opcode] = &VM::func_jsr;
        }
        else if (strcmp(mnemonic, "jnz") == 0)
        {
            opmap[opcode] = &VM::func_jnz;
        }
        else if (strcmp(mnemonic, "ret") == 0)
        {
            opmap[opcode] = &VM::func_ret;
        }
        else if (strcmp(mnemonic, "hlt") == 0)
        {
            opmap[opcode] = &VM::func_hlt;
        }
        else if (strcmp(mnemonic, "psh") == 0)
        {
            opmap[opcode] = &VM::func_psh;
        }
        else if (strcmp(mnemonic, "pop") == 0)
        {
            opmap[opcode] = &VM::func_pop;
        }
        else if (strcmp(mnemonic, "run") == 0)
        {
            opmap[opcode] = &VM::func_run;
        }
        else fprintf(stderr, "Unknown mnemonic: %s\n", mnemonic);
    }
}

#ifndef ARDUINO

int main(int argc,char* argv[])
{
    VM::buildOpMap();
    VM vm("seq.bin");
    vm.exec(0);
    // vm.dumpRegisters();
    // dumpChannels();
    return 0;
}

#endif
