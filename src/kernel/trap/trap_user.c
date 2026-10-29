#include "mod.h"
#include "../../user/syscall_num.h"

// in trampoline.S
extern char trampoline[];  // 内核和用户切换的代码
extern char user_vector[]; // 用户触发陷阱进入内核
extern char user_return[]; // 内核处理完毕返回用户

// in trap.S
extern char kernel_vector[]; // 内核态trap处理流程, 进入内核后应当切换中断处理入口

// in trap_kernel.c
extern char *interrupt_info[16]; // 中断错误信息
extern char *exception_info[16]; // 异常错误信息

// 在user_vector()里面调用
// 用户态trap处理的核心逻辑
void trap_user_handler()
{
    // 断言 trap 来自 U-mode
    assert((r_sstatus() & SSTATUS_SPP) == 0 , "trap not from U-mode");

    // 此时已处于 S-mode , 因此在 S-mode 的 trap 应该进入 kernel_vector
    w_stvec((uint64)kernel_vector);

    proc_t *proc = myproc();
    proc->tf->user_to_kern_epc = r_sepc();

    uint64 sepc = r_sepc();
    uint64 stval = r_stval();
    uint64 scause = r_scause();

    uint64 trap_id = scause & ~(1ULL << 63);

    if (scause & 0x8000000000000000ul)
    {
        // 中断
        switch (trap_id) // 中断产生原因分类
        {
            case 9:
                external_interrupt_handler();
                break;

            case 5:
                timer_interrupt_handler();
                break;

            default: // 例外处理
                printf("\nunexpected interrupt: %s\n", trap_id < 16 ? interrupt_info[trap_id] : "unknown interrupt");
                printf("trap_id = %x, sepc = %x, stval = %x\n", trap_id, sepc, stval);
                panic("trap_user_handler");
        }
    }
    else{
        // 异常
        switch (trap_id) // 异常产生原因分类
        {
            // TODO(Lab5): 增加 load/store page fault 处理。
            case 8: {
                // TODO(Lab5): 接入 syscall()；下方仅保留 Lab4 实现作为学习记录。
#if 0
                uint64 a7 = proc->tf->a7;
                switch (a7)
                {
                    case SYS_helloworld:
                        printf("proczero: hello world!\n");
                        proc->tf->a0 = 0;
                        break;
                    
                    default:
                        proc->tf->a0 = (uint64)-1;
                        break;
                }
#endif
                // 通用系统调用尚未实现，暂返回失败。
                proc->tf->a0 = (uint64)-1;
                proc->tf->user_to_kern_epc += 4;
                break;
            }

            default: // 例外处理
                printf("\nunexpected exception: %s\n", trap_id < 16 ? exception_info[trap_id] : "unknown exception");
                printf("trap_id = %x, sepc = %x, stval = %x\n", trap_id, sepc, stval);
                printf("process %d stopped; kernel remains active\n", proc->pid);
                // 本阶段没有调度器：停止用户执行，保留资源供诊断。
                // stvec 已指向 kernel_vector，等待期间仍处理内核中断。
                mycpu()->proc = NULL;
                intr_on();
                for (;;) {
                    asm volatile("wfi");
                }
        }
    }

    trap_user_return();
}

// 调用user_return()
// 内核态返回用户态
void trap_user_return()
{
    // 把 CPU 从“使用内核页表和内核栈执行内核代码”，转换为“使用用户页表和用户栈执行用户代码”
    // 准备第一次进入用户态或者是返回用户态，设定下一次陷入时怎么办

    intr_off();
    proc_t *proc = myproc();

    proc->tf->user_to_kern_satp = r_satp();
    proc->tf->user_to_kern_sp = proc->kstack + PGSIZE;
    proc->tf->user_to_kern_trapvector = (uint64)trap_user_handler;
    proc->tf->user_to_kern_hartid = r_tp();

    // stvec 寄存器保存陷入内核后执行的函数/指令的地址
    // 即指定了发生 trap 时进入 user_vector 函数
    // user_vector 函数内会调用 trap_user_handler 函数
    w_stvec(TRAMPOLINE + ((uint64)user_vector - (uint64)trampoline));

    // sepc 寄存器保存陷入内核结束后回到用户态时，执行的指令/函数
    w_sepc(proc->tf->user_to_kern_epc);

    // sstatus 寄存器保存回到用户态后的权限状态
    uint64 status = r_sstatus();
    status &= ~SSTATUS_SPP;
    status |= SSTATUS_SPIE;
    w_sstatus(status);

    // 需要计算 user_return 函数在页表中的虚拟地址，这里遵循：虚拟页表地址差等于物理页表地址差
    uint64 user_return_va = TRAMPOLINE + ((uint64)user_return - (uint64)trampoline);

    // 编码根页表的物理地址与分页格式
    uint64 user_satp = MAKE_SATP(proc->pgtbl);

    // 执行 user_return 函数，通过虚拟地址调用
    ((void (*)(uint64, uint64))user_return_va)(TRAPFRAME, user_satp);
}