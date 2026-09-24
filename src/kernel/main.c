#include "arch/mod.h"
#include "lib/mod.h"

volatile static int started = 0;

int main()
{
    if (r_tp() == 0){ // init
        print_init();
        printf("\n");
        printf("kernel is booting !\n\n");
        __sync_synchronize();
        started = 1;
    }

    while (started == 0) {}

    __sync_synchronize();

    printf("CPU %d is booting !\n" , r_tp());

    while (1) {}

    return 0;
}