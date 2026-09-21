#include "arch/mod.h"
#include "lib/mod.h"

int main()
{
    print_init();

    int id = r_tp();
    printf("CPU %d is booting !" , id);

    while (1){}

    return 0;
}