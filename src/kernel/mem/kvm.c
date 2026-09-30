#include "mod.h"

// 内核页表
static pgtbl_t kernel_pgtbl;

// 根据pagetable,找到va对应的pte
// 若设置alloc=true 则在PTE无效时尝试申请一个物理页
// 成功返回PTE, 失败返回NULL
// 提示：使用 VA_TO_VPN + PTE_TO_PA + PA_TO_PTE
pte_t *vm_getpte(pgtbl_t pgtbl, uint64 va, bool alloc)
{
    // 每一级页表的查询索引为虚拟地址 va 的对应 9 位 bit
    // 用 VA_TO_VPN(va , ) 来获取 va 中对应级别的索引
    // 通过索引查对应页表可以获得对应 PTE 页表项
    // PTE 的值的中间一段 bit 是下一级页表的首位页表项的物理地址
    // 通过 PTE_TO_PA(PTE) 获取下一级页表的首位物理地址
    // 然后再进行下一级查询

    // PTE 值的 V bit 标记是否有有效的下一级页表项
    // 如果下一级页表内存未分配且在 alloc == true 的情况下可以直接分配内存
    // 分配完内存后要更新父 PTE 的值，通过 PA_TO_PTE 函数计算出 PTE 内存的值

    pte_t *pte_2 = pgtbl + VA_TO_VPN(va , 2);                         // 查询出顶级页表的 PTE

    if (((*pte_2) & PTE_V) == 0) {   // 下级页表内存未分配
        if (!alloc)
            return NULL;
        *pte_2 = PA_TO_PTE(pmem_alloc(true)) | PTE_V;
    }
    else if (!PTE_CHECK((uint64)*pte_2)){
        return NULL;
    }

    pte_t *pte_1 = (pgtbl_t)PTE_TO_PA(*pte_2) + VA_TO_VPN(va , 1);     // 查询出次级页表的 PTE

    if ((*pte_1 & PTE_V) == 0) {
        if (!alloc)
            return NULL;
        *pte_1 = PA_TO_PTE(pmem_alloc(true)) | PTE_V;
    }
    else if (!PTE_CHECK((uint64)*pte_1)){
        return NULL;
    }

    pte_t *pte_0 = (pgtbl_t)PTE_TO_PA(*pte_1) + VA_TO_VPN(va , 0);     // 查询出低级页表的 PTE
    // 最低级页表的查询结果 PTE 不用设置 V bit

    return pte_0;
}

// 在pgtbl中建立 [va, va + len) -> [pa, pa + len) 的映射
// 本质是找到va在页表对应位置的pte并修改它
// 检查: va pa 应当是 page-aligned, len(字节数) > 0, va + len <= VA_MAX
// 注意: perm 应该如何使用
void vm_mappages(pgtbl_t pgtbl, uint64 va, uint64 pa, uint64 len, int perm)
{
    // 本质就是将虚拟地址 va 对应的末级 PTE 的值设为 PA_TO_PTE(pa) 然后再根据 perm 修改 XRW 权限
    // v bit 的值显示设置为 1

    assert(((va - (uint64)ALLOC_BEGIN) % (uint64)PGSIZE == 0) , "Invalid VA");
    assert(((pa - (uint64)ALLOC_BEGIN) % (uint64)PGSIZE == 0) , "Invalid PA");
    assert((len > 0 && va <= VA_MAX && len <= VA_MAX && (va + len <= VA_MAX && va + len >= va && va + len >= len)) , "Invalid len");

    for (uint64 i = va ; i <= (uint64)((va + len - 1) - (va + len - 1) % (uint64)PGSIZE) ; i+=(uint64)PGSIZE) {
        pte_t *pte = vm_getpte(pgtbl , i , true);

        assert((pte != NULL) , "Getpte Failed");

        *pte = PA_TO_PTE(pa + i - va);

        (*pte) = ((*pte) & (~PTE_R)) | (perm & PTE_R);
        (*pte) = ((*pte) & (~PTE_W)) | (perm & PTE_W);
        (*pte) = ((*pte) & (~PTE_X)) | (perm & PTE_X);
        (*pte) = ((*pte) & (~PTE_V)) | (1 & PTE_V);
    }
}

// 解除pgtbl中[va, va+len)区域的映射
// 如果freeit == true则释放对应物理页, 默认是用户的物理页
void vm_unmappages(pgtbl_t pgtbl, uint64 va, uint64 len, bool freeit)
{
    assert(((va - (uint64)ALLOC_BEGIN) % (uint64)PGSIZE == 0) , "Invalid VA");
    assert((len > 0 && va <= VA_MAX && len <= VA_MAX && (va + len <= VA_MAX && va + len >= va && va + len >= len)) , "Invalid len");
    
    for (uint64 i = va ; i <= (uint64)((va + len - 1) - (va + len - 1) % (uint64)PGSIZE) ; i+=(uint64)PGSIZE) {
        pte_t *pte = vm_getpte(pgtbl , i , false);

        assert(pte != NULL , "Getpte Failed");

        if ((((uint64)*pte) & PTE_V) == 0) { // 本来就无效，不用释放
            *pte = 0;
            continue;
        }

        if (freeit)
            pmem_free((uint64)PTE_TO_PA((uint64)*pte) , false); // 因为默认用户物理页所以是 false

        *pte = 0;
    }
}

// 完成UART、PLIC、内核代码区、内核数据区、可分配区域的页表映射
// CLINT属于M-mode资源，OpenSBI版内核通过SBI管理timer/IPI，不直接映射。
// 相当于部分填充kernel_pgtbl
void kvm_init()
{
    // 要映射的区域：
    // UART : 0x10000000ul(UART_BASE) ~ (UART_BASE + PGSIZE)
    // PLIC : 0x0c000000ul(PLIC_BASE) ~ 0x0c000000ul + 0x04000000ul(PLIC_BASE + PLIC_SIZE)
    // 内核代码 : KERNEL_BASE ~ KERNEL_DATA
    // 内核数据 : KERNEL_DATA ~ ALLOC_BEGIN
    // 可分配区域 : ALLOC_BEGIN ~ ALLOC_END

    // 以上全部使用 VA == PA 的对等映射

    kernel_pgtbl = pmem_alloc(true);

    vm_mappages(kernel_pgtbl , (uint64)UART_BASE , (uint64)UART_BASE , (uint64)PGSIZE , PTE_R | PTE_W);
    vm_mappages(kernel_pgtbl , (uint64)PLIC_BASE , (uint64)PLIC_BASE , (uint64)PLIC_SIZE , PTE_R | PTE_W);
    vm_mappages(kernel_pgtbl , (uint64)KERNEL_BASE , (uint64)KERNEL_BASE , (uint64)KERNEL_DATA - (uint64)KERNEL_BASE , PTE_R | PTE_X);
    vm_mappages(kernel_pgtbl , (uint64)KERNEL_DATA , (uint64)KERNEL_DATA , (uint64)ALLOC_END - (uint64)KERNEL_DATA , PTE_R | PTE_W);
}

// 每个CPU都需要调用, 从不使用页表切换到使用内核页表
// 切换后需要刷新TLB里面的缓存
void kvm_inithart()
{
    w_satp(MAKE_SATP(kernel_pgtbl));
    sfence_vma();
}

// 输出页表内容(for debug)
void vm_print(pgtbl_t pgtbl)
{
    // 顶级页表，次级页表，低级页表
    pgtbl_t pgtbl_2 = pgtbl, pgtbl_1 = NULL, pgtbl_0 = NULL;
    pte_t pte;

    printf("level-2 pgtbl: pa = %p\n", pgtbl_2);
    for (int i = 0; i < PGSIZE / sizeof(pte_t); i++)
    {
        pte = pgtbl_2[i];
        if (!((pte)&PTE_V))
            continue;
        assert(PTE_CHECK(pte), "vm_print: pte check fail (1)");
        pgtbl_1 = (pgtbl_t)PTE_TO_PA(pte);
        printf(".. level-1 pgtbl %d: pa = %p\n", i, pgtbl_1);

        for (int j = 0; j < PGSIZE / sizeof(pte_t); j++)
        {
            pte = pgtbl_1[j];
            if (!((pte)&PTE_V))
                continue;
            assert(PTE_CHECK(pte), "vm_print: pte check fail (2)");
            pgtbl_0 = (pgtbl_t)PTE_TO_PA(pte);
            printf(".. .. level-0 pgtbl %d: pa = %p\n", j, pgtbl_0);

            for (int k = 0; k < PGSIZE / sizeof(pte_t); k++)
            {
                pte = pgtbl_0[k];
                if (!((pte)&PTE_V))
                    continue;
                assert(!PTE_CHECK(pte), "vm_print: pte check fail (3)");
                printf(".. .. .. physical page %d: pa = %p flags = %d\n", k, (uint64)PTE_TO_PA(pte), (int)PTE_FLAGS(pte));
            }
        }
    }
}
