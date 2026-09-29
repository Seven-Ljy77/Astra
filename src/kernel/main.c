#include "arch/mod.h"
#include "lib/mod.h"
#include "mem/mod.h"

volatile static int started = 0;

volatile static int main_hart = -1;

extern void _entry(void);

int main()
{
    int cpuid = r_tp();

    // 检查旧值如果符合 -1 则交换为 cpuid (原子操作)
    // 返回是否交换成功
    bool if_main_hart = __sync_bool_compare_and_swap(&main_hart, -1, cpuid);

    if (if_main_hart) {
        print_init();
        pmem_init();

        printf("\n");
        printf("kernel is booting !\n");
        printf("\n");

        __sync_synchronize();
        started = 1;

        printf("CPU %d is booting !\n" , cpuid);

        for (int i=0 ; i<(int)NCPU ; i++) {
            if (i == cpuid)
                continue;

            sbi_ret_t result = sbi_hart_start((uint64)i , (uint64)_entry , (uint64)0);
            if (result.error != (int64)SBI_SUCCESS) {
                printf("Hart %d Starting Failed , result.error = %d" , i , (int)(result.error));
                return 0;
            }
        }
    }
    else {
        while (started == 0) {}
        __sync_synchronize();
        
        printf("CPU %d is booting !\n" , cpuid);   
    }

    while (1) {}

    return 0;
}