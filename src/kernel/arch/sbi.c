#include "mod.h"
// 通过opensbi与底层硬件交互
sbi_ret_t sbi_ecall(uint64 eid, uint64 fid,
                    uint64 arg0, uint64 arg1, uint64 arg2,
                    uint64 arg3, uint64 arg4, uint64 arg5)
{
  register uint64 a0 asm("a0") = arg0;
  register uint64 a1 asm("a1") = arg1;
  register uint64 a2 asm("a2") = arg2;
  register uint64 a3 asm("a3") = arg3;
  register uint64 a4 asm("a4") = arg4;
  register uint64 a5 asm("a5") = arg5;
  register uint64 a6 asm("a6") = fid;
  register uint64 a7 asm("a7") = eid;

  asm volatile("ecall"
               : "+r"(a0), "+r"(a1)
               : "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                 "r"(a6), "r"(a7)
               : "memory");

  return (sbi_ret_t){(int64)a0, (int64)a1};
}

// 用于在已启动第一个内核的情况下，再启动一个内核
// 这里的第二个参数 start_addr 是 启动代码 的地址，即启动 hart 的一段汇编的第一行命令的地址
// 也就是 _entry 标签指示的位置
// 第三个参数这个 lab2 好像用不到
sbi_ret_t sbi_hart_start(uint64 hartid, uint64 start_addr, uint64 opaque)
{
  return sbi_ecall(SBI_EXT_HSM, SBI_HSM_HART_START,
                   hartid, start_addr, opaque, 0, 0, 0);
}
