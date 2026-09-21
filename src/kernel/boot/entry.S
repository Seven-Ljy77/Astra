# kernel.ld 将_entry作为整个OS的起点放置到0x80000000处
# qemu会自动跳转到0x80000000处并开始执行
# 注意: 此时是M-mode

.section .text
.global _entry
_entry:
        # CPU_stack 定义于start.c中
        # sp = CPU_stack + ((hartid + 1) * 4096)
        # 将sp置于当前CPU的内核栈的栈顶
        la sp, CPU_stack        # 把 CPU_stack 数组的首位地址值写入变量 sp 中
        li a0, 4096             # a0 = 4096
        csrr a1, mhartid        # a1 = mhartid  读取控制状态
        addi a1, a1, 1          # a1 = a1 + 1
        mul a0, a0, a1          # a0 = a0 * a1 = 4096 * (mhartid + 1)   CPU0:a0=4096    CPU1:a0=8192
        add sp, sp, a0          # sp = sp + a0  相当于加到栈数组的起始位置，为 CPU 栈地址的最大值
        # 跳转到start
        call start
spin:
        j spin
