#include <stdio.h>

int main()
{
    long instr = 0;
    fread(&instr, 3, 1, stdin);
    printf("%lx\n", instr);
}
