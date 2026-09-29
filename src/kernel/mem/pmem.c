#include "mod.h"

// 内核空间和用户空间的可分配物理页分开描述
static alloc_region_t kern_region, user_region;

// 物理内存的初始化
// 本质上就是填写kern_region和user_region, 包括基本数值和空闲链表
void pmem_init(void)
{
    kern_region.begin = (uint64)ALLOC_BEGIN;
    kern_region.end = (uint64)(ALLOC_BEGIN + KERN_PAGES * PGSIZE);
    kern_region.allocable = (uint32)(KERN_PAGES);
    spinlock_init(&(kern_region.lk) , (char *)"kern_region_lk");
    kern_region.list_head.next = (page_node_t *)kern_region.begin; // list_head 不作为第一个空闲页，list_head.next 才指向第一个空闲页

    page_node_t *ptr = kern_region.list_head.next; // 第一个空闲页的指针
    for (int i=1 ; i<(int)(kern_region.allocable) ; i++) {
        ptr->next = (page_node_t *)((uint8 *)ptr + PGSIZE);
        ptr = ptr->next;
    }
    ptr->next = NULL;

    user_region.begin = (uint64)(kern_region.end);
    user_region.end = (uint64)(ALLOC_END);
    user_region.allocable = (uint32)(((uint64)ALLOC_END - (uint64)ALLOC_BEGIN) / PGSIZE - kern_region.allocable);
    spinlock_init(&(user_region.lk) , (char *)"user_region_lk");
    user_region.list_head.next = (page_node_t *)(user_region.begin);

    ptr = user_region.list_head.next;
    for (int i=1 ; i<(int)(user_region.allocable) ; i++) {
        ptr->next = (page_node_t *)((uint8 *)ptr + PGSIZE);
        ptr = ptr->next;
    }
    ptr->next = NULL;
}

// 尝试返回一个可分配的清零后的物理页
// 失败则panic锁死
void *pmem_alloc(bool in_kernel)
{
    page_node_t *page;

    return page;
}

// 释放一个物理页
// 失败则panic锁死
void pmem_free(uint64 page, bool in_kernel)
{
}
