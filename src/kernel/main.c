#include "arch/mod.h"
#include "lib/mod.h"

volatile static int started = 0;

volatile static int main_hart = -1;

extern void _entry(void);

int main()
{
    int cpuid = r_tp();

    if (main_hart == -1)
        main_hart = cpuid;

    if (cpuid == main_hart) {
        print_init();

        printf("\n");
        printf("kernel is booting !\n");
        printf("\n");

        __sync_synchronize();
        started = 1;

        for (int i=0 ; i<(int)NCPU ; i++) {
            if (i == cpuid)
                continue;

            sbi_ret_t result = sbi_hart_start((uint64)i , (uint64)_entry , (uint64)0);
            if (result.error != (int64)SBI_SUCCESS) {
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