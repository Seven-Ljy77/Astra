#include "../arch/mod.h"

// 每个CPU在运行操作系统时需要一个初始的函数栈
__attribute__((aligned(16))) uint8 CPU_stack[4096 * NCPU];

extern int main();

// entry.S 中负责参数传入，a0 a1 寄存器作为参数，这里只写一个参数表示只接受 a0 寄存器的参数
// a0 寄存器为 64 位寄存器，因此使用 uint64
// openSBI 固件会将 cpuid 写入 a0 寄存器，再由 entry.S 将 a0 作为参数传入 start 函数
void start(uint64 hartid)
{
    // 暂时不开启分页，使用物理地址
    w_satp(0);

    // 将hartid存到可访问的寄存器tp，让 main 函数读取
    w_tp(hartid);

    main();
}