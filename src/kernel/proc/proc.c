#include "mod.h"

// 这个文件通过make build生成, 是proczero对应的ELF文件
#include "../../user/initcode.h"
#define initcode target_user_initcode
#define initcode_len target_user_initcode_len

// in trampoline.S
extern char trampoline[];

// in swtch.S
extern void swtch(context_t *old, context_t *new);

// in trap/trap_user.c
extern void trap_user_return();

// 第一个用户进程
static proc_t proczero;

// 获得一个初始化过的用户页表
// 完成trapframe和trampoline的映射
pgtbl_t proc_pgtbl_init(uint64 trapframe)
{
    // 每个进程有自己独立的页表，例如 proczero.pgtbl
    // 这样不同进程可以访问相同的虚拟地址但是不同的物理地址
    // 这个函数只返回一个初始化后的页表首地址，不直接修改 proczero.pgtbl

    pgtbl_t pgtbl = pmem_alloc(true);

    vm_mappages(pgtbl , TRAMPOLINE , (uint64)trampoline , PGSIZE , PTE_R | PTE_X);
    vm_mappages(pgtbl , TRAPFRAME , trapframe , PGSIZE , PTE_R | PTE_W);

    return pgtbl;
}

/*
    第一个用户态进程的创建
    它的代码和数据位于initcode.h的initcode数组

    第一个进程的用户地址空间布局:
    trapoline   (1 page)
    trapframe   (1 page)
    ustack      (1 page)
    .......
                        <--heap_top
    code + data (1 page)
    empty space (1 page) 最低的4096字节 不分配物理页，同时不可访问

	注意: 用用户空间的地址映射需要标记 PTE_U
*/
void proc_make_first()
{
    // 设置 pid
    proczero.pid = 0;

    // 申请 trapframe 物理页并同时映射 trapframe 和 trampoline
    trapframe_t *trapframe = pmem_alloc(true);
    proczero.pgtbl = proc_pgtbl_init((uint64)trapframe);
    proczero.tf = trapframe;

    // 按要求预先分配一页用户栈，前两页分别是 trampoline 和 trapframe，因此这个用户栈用第三页
    void *ustack = pmem_alloc(false);
    vm_mappages(proczero.pgtbl , (VA_MAX - 3 * PGSIZE) , (uint64)ustack , PGSIZE , PTE_R | PTE_W | PTE_U);

    // 记录已经分配的用户栈数量
    proczero.ustack_npage = 1;

    // user.ld 指定用户程序从 0x1000 开始，这里的 0x1000 是虚拟地址
    proczero.heap_top = 0x2000;

    // 为用户代码 elf 申请一个页，并复制用户代码到这个页
    void *elf_data_pg = pmem_alloc(false);
    vm_mappages(proczero.pgtbl , 0x1000 , (uint64)elf_data_pg , PGSIZE , PTE_R | PTE_X | PTE_U);
    assert(initcode_len <= PGSIZE, "initcode too large");
    memmove(elf_data_pg , initcode , initcode_len);

    // sp 字段决定用户进程第一次运行时，从哪里开始运行，因此 sp 填用户栈的栈顶指针
    trapframe->sp = VA_MAX - 2 * PGSIZE;

    // user_to_kern_epc 字段填写用户程序入口
    trapframe->user_to_kern_epc = 0x1000;

    // KSTACK 为进程陷入内核后使用的内核内存中的栈，在本 lab4 中，多个进程共同使用同一个内核页表
    proczero.kstack = KSTACK(0); // 内核栈第一个页的虚拟地址
    proczero.ctx.sp = proczero.kstack + PGSIZE;
    proczero.ctx.ra = (uint64)trap_user_return; // 返回时执行这个函数

    mycpu()->proc = &proczero;

    // 进入 swtch 函数
    // 因为 ra 设置了 trap_user_return ，因此 swtch 结束后进入 trap_user_return 函数
    swtch(&mycpu()->ctx , &proczero.ctx);   
}