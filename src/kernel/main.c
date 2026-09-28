#include "arch/mod.h"
#include "lib/mod.h"

volatile static int started = 0;

extern void _entry();

int main()
{
    uint64 cpuid = r_tp();

    if (cpuid == 0) {
        print_init();

        printf("\n");
        printf("kernel is booting !\n");
        printf("\n");

        __sync_synchronize();
        started = 1;

        for (uint64 i=0 ; i<(uint64)NCPU ; i++) {
            if (i == cpuid)
                continue;

            sbi_ret_t result = sbi_hart_start(i , (uint64)_entry , 0);
            if (result.error != SBI_SUCCESS) {
                printf("Hart %d Starting Failed" , i);
                return 0;
            }
        }
    }

    while (started == 0) {}

    printf("CPU %d is booting !\n" , cpuid);

    while (1) {}

    return 0;
}