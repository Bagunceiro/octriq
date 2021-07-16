#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include "seqlang.h"
#include "seqvm.h"

int main(int argc,char* argv[])
{
    VM::buildOpMap();
    VM vm("seq.bin");
    vm.exec(0);
    return 0;
}
