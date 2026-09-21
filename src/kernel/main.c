#include "arch/mod.h"
#include "lib/mod.h"

int main()
{
    print_init();

    printf("CPU %d is booting !\n" , r_tp());

    while (1){}

    return 0;
}