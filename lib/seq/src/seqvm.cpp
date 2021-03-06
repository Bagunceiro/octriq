#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include "seqlang.h"
#include "seqvm.h"
#include "hardware.h"

#ifdef ARDUINO
#include <LITTLEFS.h>
#else
#include <getopt.h>

unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void delay(unsigned int msecs)
{
    unsigned int usecs = msecs * 1000;
    fflush(stdout);
    usleep(usecs);
}

#endif

Text::Text()
{
    size = 0;
    memory = NULL;
}

Text &Text::operator=(const Text &rhs)
{
    memory = rhs.memory;
    size = rhs.size;
    return *this;
}

Text::Text(const Text &rhs)
{
    *this = rhs;
}

Text::~Text()
{
}

unsigned int Text::load(File f)
{
    SEEKFILE(f, 0, SEEKMODE_END);
    unsigned int bytes = POSINFILE(f);
    size = bytes / INSTR_BYTES;
    std::shared_ptr<uint32_t> sh1(new uint32_t[size]);
    memory = sh1;
    SEEKFILE(f, 0, SEEKMODE_SET);
    for (int i = 0; i < size; i++)
    {
        READFILE(f, &(memory.get()[i]), INSTR_BYTES);
    }
    return size;
}

class Instruction
{
public:
    Instruction(const unsigned long ins);
    virtual ~Instruction();
    // unsigned int timecode() { return _timecode; }
    unsigned int opcode() { return _opcode; }
    unsigned int op1() { return _op1; }
    unsigned int ind() { return _ind; }
    unsigned int op2() { return _op2; }
    void dump();

private:
    void deconstruct(const unsigned long ins);
    unsigned int getField(unsigned long code, int width, int pos, int bits);
    // unsigned int _timecode;
    unsigned int _opcode;
    unsigned int _op1;
    unsigned int _ind;
    unsigned int _op2;
};

void Instruction::dump()
{
    LOGF("  %02x %02x %01x %02x\n", _opcode, _op1, _ind, _op2);
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
        mask <<= 1;
        mask |= 1;
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
    //    _timecode = getField(code, INSTR_BITS, OFFSET_TC, WIDTH_TC);
    _opcode = getField(code, INSTR_BITS, OFFSET_OPC, WIDTH_OPC);
    _op1 = getField(code, INSTR_BITS, OFFSET_OP1, WIDTH_OP1);
    _ind = getField(code, INSTR_BITS, OFFSET_IND, WIDTH_IND);
    _op2 = getField(code, INSTR_BITS, OFFSET_OP2, WIDTH_OP2);
}

void dumpChannels()
{
    for (int i = 0; i < NCHANNELS; i++)
    {
        Channel *ch = Channel::getChannel(i);
        if (ch->valid())
            LOGF("Channel %x = %d\n", i, ch->get());
    }
}

std::map<int, int (VM::*)(int, int)> VM::opmap;
// typdef int(VM::*opfunc)(int,int);
std::map<int, VM *> VM::tasklist;

void VM::start(int address)
{
    jumpto(address);
    exec();
}

int VM::kill(int vmno)
{
    int num = vmno;
    VM *vm = tasklist[vmno];
    if (vm)
    {
        vm->halt = true;
    }
    return num;
}

int killvm(int jobno)
{
    int res = VM::kill(jobno);
    return res;
}

#ifdef ARDUINO
int VM::listTasks(Print &out)
{
    for (auto const &x : tasklist)
    {
        VM *vm = x.second;
        int dutyCycle;
        unsigned long elapsed = millis() - vm->started;
        unsigned long runtime = elapsed - vm->totalsleep;
        dutyCycle = (runtime * 100) / elapsed;

        out.printf("%d, %s, duty cycle=(%lu/%lu) %d%%, overuns=%d\n",
                   x.first, vm->name,
                   runtime, elapsed, dutyCycle,
                   vm->countOverruns);
    }
    return 0;
}

int VM::taskCount()
{
    return tasklist.size();
}

int VM::setTrace(int vmnum, bool value)
{
    VM *vm = tasklist[vmnum];
    if (vm)
    {
        vm->setTrace(value);
        return vmnum;
    }
    return -1;
}

void vmsetTrace(int vmnum, bool value)
{
    VM::setTrace(vmnum, value);
}

int listvms(Print &out)
{
    return VM::listTasks(out);
}

#endif

int VM::numVMs = 0;

void VM::startAsTask(int address, int stacksize)
{
    progCounter = address;
#ifdef ARDUINO
    char buff[8];
    vmnumber = ++numVMs;
    tasklist[vmnumber] = this;
    snprintf(buff, sizeof(buff) - 1, "VM%d", numVMs);
    xTaskCreate(
        doExec,
        buff,
        stacksize,
        this,
        tskIDLE_PRIORITY,
        &xHandle);
#ifdef TUNE_STACK_SIZE
    minhwm = uxTaskGetStackHighWaterMark(xHandle);
#endif
#else
    // Create a thread here
    exec();
#endif
}

#ifdef ARDUINO

void VM::doExec(void *ptr)
{
    VM *vm = reinterpret_cast<VM *>(ptr);

    vm->exec();
    Serial.flush();
    tasklist.erase(vm->vmnumber);
    TaskHandle_t t = vm->xHandle;
    delete (vm);

    vTaskDelete(t);
}

void VM::createTask(const char *name, int stacksize)
{
}

#endif

VM::VM(File f, int stk)
{
    //    binfile = f;
    zero = 0;
    trace = false;
    progCounter = 0;
    txt.load(f);
    vmnumber = 0;
#ifdef ARDUINO
    xHandle = NULL;
#endif
    halt = false;
    name = strdup("");
    countOverruns = 0;
}

VM &VM::operator=(const VM &rhs)
{
    txt = rhs.txt;
    halt = false;
    trace = rhs.trace;
    progCounter = 0;
    vmnumber = 0;
#ifdef ARDUINO
    xHandle = NULL;
#endif
    zero = 0;
    name = strdup("");
    countOverruns = 0;
    return *this;
}

VM::VM(const VM &rhs)
{
    *this = rhs;
}

VM::~VM()
{
    free(name);
}

void VM::push(unsigned int val)
{
    stack.push_back(val);
}

unsigned int VM::pop()
{
    int result = 0;
    if (!stack.empty())
    {
        result = stack.back();
        stack.pop_back();
    }
    else
    {
        LOGF("Stack Underflow at %d\n", progCounter);
        result = 0;
        halt = true;
    }
    return result;
}

bool VM::fetch(unsigned long *val)
{
    *val = txt[progCounter];
    if (progCounter < txt.getSize())
    {
        progCounter++;
        return true;
    }
    return false;
}

void VM::jumpto(int address)
{
    progCounter = address;
}

void VM::exec()
{
    started = millis();
    due = started;
    totalsleep = 0;

    jumpto(progCounter);

    while (true)
    {
#ifdef TUNE_STACK_SIZE
        if (xHandle)
        {
            int hwm = uxTaskGetStackHighWaterMark(xHandle);
            if (hwm < minhwm)
            {
                LOGF("VM%d: HWM = %d\n", vmnumber, hwm);
                minhwm = hwm;
            }
        }
#endif
        unsigned int tmpadd = progCounter;
        unsigned long insbin;

        if (halt)
            break;

        if (fetch(&insbin))
        {
            Instruction in(insbin);

            int (VM::*func)(int, int) = opmap[in.opcode() & OPC_MASK];

            if (trace)
                LOGF("%10ld %04x: ", millis() - started, tmpadd);
            if (func)
            {
                int lval = in.op1();
                if (in.opcode() & IND_MASK)
                {
                    // lval is indirect (actual lval is held in %lval)
                    // lval &= REG_MASK;

                    if (lval & REGIND_MASK)
                    {
                        lval &= REG_MASK;
                        lval = reg[lval] | REGIND_MASK;
                    }
                    else
                    {
                        lval = reg[lval];
                    }
                }
                int rval = in.op2();
                if (in.ind())
                {
                    if (rval > NCHANNELS)
                        rval = reg[rval & REG_MASK];
                    else
                    {
                        rval = Channel::getChannel(rval)->get();
                    }
                }
                if ((this->*func)(lval, rval) < 0)
                    break;
                ;
            }
            else
            {
                LOGF("Unknown opcode %x at %d\n", in.opcode(), progCounter);
                halt = true;
            }
        }
        else
            break; // fallen off the end;
    }
}

bool isreg(int *index)
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
    if (trace)
        LOGF("NOP\n");
    return 0;
}

int VM::func_set(int lval, int rval)
{
    bool lvalreg = isreg(&lval);
    if (trace)
        LOGF("SET %s%d to %d\n", (lvalreg ? "%" : "#"), lval, rval);
    if (lvalreg)
    {
        reg[lval] = rval;
    }
    else
    {
        Channel::getChannel(lval)->set(rval);
    }
    return 0;
}

int VM::func_clr(int, int)
{
    if (trace)
        LOGF("CLR\n");
    for (int i = 0; i < NCHANNELS; i++)
    {
        Channel::getChannel(i)->set(0);
    }
    return 0;
}

int VM::clr()
{
    for (int i = 0; i < NCHANNELS; i++)
    {
        Channel::getChannel(i)->set(0);
    }
    return 0;
}

int VM::func_add(int lval, int rval)
{
    bool lvalreg = isreg(&lval);
    overflow = false;
    zero = false;
    if (trace)
        LOGF("ADD %d to %s%d ", rval, (lvalreg ? "%" : "#"), lval);
    if (lvalreg)
    {
        int v = reg[lval] + rval;
        int vmod = v % MOD_OP2;
        if (v != vmod)
            overflow = true;
        reg[lval] = vmod;
        if (trace)
            LOGF("(=%d)\n", reg[lval]);
        zero = (reg[lval] == 0);
    }
    else
    {
        Channel *ch = Channel::getChannel(lval);
        int v = ch->get() + rval;
        int vmod = v % MOD_OP2;
        if (v != vmod)
            overflow = true;
        ch->set(vmod);
        if (trace)
            LOGF("(=%d)\n", ch->get());
        zero = (ch->get() == 0);
    }

    return 0;
}

int VM::func_sub(int lval, int rval)
{
    overflow = false;
    zero = false;
    bool lvalreg = isreg(&lval);
    if (trace)
        LOGF("SUB %d from %s%d", rval, (lvalreg ? "%" : "#"), lval);
    if (lvalreg)
    {
        int v = reg[lval] - rval;
        if (v < 0)
        {
            v += MOD_OP2;
            overflow = true;
        }
        reg[lval] = v;
        if (trace)
            LOGF("(=%d)\n", reg[lval]);
        zero = (reg[lval] == 0);
    }
    else
    {
        Channel *ch = Channel::getChannel(lval);
        if (ch)
        {
            int v = ch->get() - rval;
            if (v < 0)
            {
                v += MOD_OP2;
                overflow = true;
            }
            ch->set(v);
            if (trace)
                LOGF("(=%d)\n", ch->get());
            zero = (ch->get() == 0);
        }
        else
            LOGF("\n");
    }
    return 0;
}

int VM::func_jmp(int, int rval)
{
    jumpto(rval);
    if (trace)
        LOGF("JMP to %d\n", rval);
    return 0;
}

int VM::func_jsr(int, int rval)
{
    push(progCounter);
    jumpto(rval);
    if (trace)
        LOGF("JSR to %d\n", rval);
    return 0;
}

int VM::func_jnz(int, int rval)
{
    if (!zero)
    {
        jumpto(rval);
        if (trace)
            LOGF("JNZ to %d\n", rval);
    }
    else
    {
        if (trace)
            LOGF("JNZ ----\n");
    }
    return 0;
}

int VM::func_jz(int, int rval)
{
    if (zero)
    {
        jumpto(rval);
        if (trace)
            LOGF("JZ to %d\n", rval);
    }
    else
    {
        if (trace)
            LOGF("JZ ----\n");
    }
    return 0;
}

int VM::func_jnv(int, int rval)
{
    if (!overflow)
    {
        jumpto(rval);
        if (trace)
            LOGF("JNV to %d\n", rval);
    }
    else
    {
        if (trace)
            LOGF("JNV ----\n");
    }
    return 0;
}

int VM::func_jv(int, int rval)
{
    if (overflow)
    {
        jumpto(rval);
        if (trace)
            LOGF("JV to %d\n", rval);
    }
    else
    {
        if (trace)
            LOGF("JV ----\n");
    }
    return 0;
}

int VM::func_ret(int, int)
{
    progCounter = pop();
    jumpto(progCounter);
    if (trace)
        LOGF("RET (to %d)\n", progCounter);
    return 0;
}

int VM::func_hlt(int, int)
{
    if (trace)
        LOGF("HLT\n");
    return -1;
}

int VM::func_psh(int lval, int)
{
    bool lvalreg = isreg(&lval);
    int value = 0;
    if (lvalreg)
    {
        value = reg[lval];
    }
    else
    {
        value = Channel::getChannel(lval)->get();
    }
    push(value);
    if (trace)
        LOGF("PSH %s%d (%d)\n", (lvalreg ? "%" : "#"), lval, value);
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
        Channel::getChannel(lval)->set(value);
    }
    if (trace)
        LOGF("POP %s%d (%d)\n", (lvalreg ? "%" : "#"), lval, value);

    return 0;
}

int VM::func_run(int, int rval)
{
    if (trace)
        LOGF("RUN at %d\n", rval);
    VM *vm = new VM(*this);
    // vm->settrace(true);
    vm->startAsTask(rval);
    return 0;
}

int VM::func_dly(int, int rval)
{
    unsigned long now = millis();
    due = due + rval;
    long sleeptime = (due - now);
    if (trace)
        LOGF("DLY %d (actual: %ld)\n", rval, sleeptime);

    if ((sleeptime > 0) && (sleeptime < 1024))
    {
        totalsleep += sleeptime;
        delay(sleeptime);
    }
    else
    {
        countOverruns++;
        if (countOverruns > 20)
        {
            countOverruns = 0;
            (LOGF("time overrun (%lu - %lu = %ld)\n", due, now, sleeptime));
            // Reset timing
            due = now;
        }
    }

    static unsigned long pause = 0;
    if ((now - pause) > 600000) // Every 10 minutes
    {
        pause = now;
        LOGF("%lu: started %lu, runtime %lu, totalsleep %lu\n", now, started, now - started, totalsleep);
    }

    return 0;
}

int VM::func_cmp(int lval, int rval)
{
    // !!!! This is more or less identical to func_sub
    // change it to share the code.
    overflow = false;
    zero = false;
    bool lvalreg = isreg(&lval);

    if (trace)
        LOGF("CMP %s%d, %d", (lvalreg ? "%" : "#"), lval, rval);
    if (lvalreg)
    {
        int v = reg[lval] - rval;
        if (v < 0)
        {
            v += MOD_OP2;
            overflow = true;
        }
        //        reg[lval] = v;
        //        if (trace)
        //            LOGF("(=%d)\n", reg[lval]);
        zero = (v == 0);
    }
    else
    {
        Channel *ch = Channel::getChannel(lval);
        if (ch)
        {
            int v = ch->get() - rval;
            if (v < 0)
            {
                v += MOD_OP2;
                overflow = true;
            }
            //            ch->set(v);
            //            if (trace)
            //                LOGF("(=%d)\n", ch->get());
            zero = (v == 0);
        }
        else
            LOGF("\n");
    }
    return 0;
}

void VM::buildOpMap()
{
    const char *mnemonic;
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
        else if (strcmp(mnemonic, "jz") == 0)
        {
            opmap[opcode] = &VM::func_jz;
        }
        else if (strcmp(mnemonic, "jnv") == 0)
        {
            opmap[opcode] = &VM::func_jnv;
        }
        else if (strcmp(mnemonic, "jv") == 0)
        {
            opmap[opcode] = &VM::func_jv;
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
        else if (strcmp(mnemonic, "dly") == 0)
        {
            opmap[opcode] = &VM::func_dly;
        }
        else if (strcmp(mnemonic, "cmp") == 0)
        {
            opmap[opcode] = &VM::func_cmp;
        }
        else
            LOGF("BuildOpMap: Unknown mnemonic: %s\n", mnemonic);
    }
    Channel::buildList();
}

#ifdef ARDUINO

int VM::runBinary(const char *filename)
{
    int result = -1;
    File f = OPENFILE(filename, "r");
    if (f)
    {
        VM *vm = new VM(f);
        vm->setName(filename);
        // vm->settrace(true);
        vm->startAsTask(0);
        result = vm->getNumber();
    }
    else
    {
        LOGF("Cannot open %s\n", filename);
    }
    return result;
}

#else
int main(int argc, char *argv[])
{
    int opt;
    int exitcode = 0;
    bool trace = false;

    while ((opt = getopt(argc, argv, "t")) != -1)
    {
        switch (opt)
        {
        case 'n':
            break;
        case 't':
            trace = true;
            break;
        default: /* '?' */
            exitcode = -1;
        }
    }
    if ((optind >= argc) || (exitcode < 0))
    {
        LOGF("Usage: [-t] %s BINARYFILE\n", argv[0]);
        exitcode = -1;
    }
    else
    {
        VM::buildOpMap();
        File fp = OPENFILE(argv[optind], "r");
        if (fp)
        {
            VM vm(fp);
            vm.setTrace(trace);
            vm.start();
            CLOSEFILE(fp);
        }
        else
        {
            LOGF("Could not open binary file %s\n", argv[optind]);
        }
    }
    return exitcode;
}

#endif
