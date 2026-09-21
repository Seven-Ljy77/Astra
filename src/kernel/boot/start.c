#include "../arch/mod.h"

// 每个CPU在运行操作系统时需要一个初始的函数栈
__attribute__((aligned(16))) uint8 CPU_stack[4096 * NCPU];

extern void main();

void start()
{
    // 暂时不开启分页，使用物理地址
    w_satp(0);

    // 切换到S-mode后无法访问M-mode的寄存器
    // 所以需要将hartid存到可访问的寄存器tp
    int id = r_mhartid();
    w_tp(id);

    // 修改mstatus寄存器，假装上一个状态是S-mode
    uint64 status = r_mstatus();   // mstatus 寄存器有一个 MPP 字段为第 11 , 12 位置 bit , 用来记录切换的下一个状态
    status &= ~MSTATUS_MPP_MASK;   // 先把 11 , 12 两个 bit 清零
    status |= MSTATUS_MPP_S;       // 再把这两个 bit 的值设置为 S-Mode 应有的值
    w_mstatus(status);             // 将新值写入寄存器

    // 设置M-mode的返回地址
    w_mepc((uint64)main);
    // 触发状态迁移，回到上一个状态（M-mode->S-mode）
    asm volatile("mret");
}