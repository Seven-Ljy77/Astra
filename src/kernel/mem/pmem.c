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
void *pmem_alloc(bool in_kernel) // true : 从 kern_region 分配      false : 从 user_region 分配
{
    alloc_region_t *region = in_kernel ? &kern_region : &user_region;

    spinlock_acquire(&region->lk);  // 必须放在读取 *page 前，否则多个核会同时拿到同一个 page 地址

    page_node_t *page = region->list_head.next;

    if (page == NULL) {
        spinlock_release(&region->lk);
        assert(false , (char *)((in_kernel) ? "Kernel Memory Exhausted" : "User Memory Exhausted"));
        return NULL;
    }
    
    region->list_head.next = page->next;
    region->allocable --;

    spinlock_release(&region->lk);

    memset((void *)page , (uint8)0 , (uint32)PGSIZE);

    return page;
}

// 释放一个物理页
// 失败则panic锁死
void pmem_free(uint64 page, bool in_kernel)
{
    alloc_region_t *region = in_kernel ? &kern_region : &user_region;

    spinlock_acquire(&region->lk);

    page_node_t *ptr = region->list_head.next; // 起始页节点

    if (page < region->begin || page > region->end - PGSIZE || (page - region->begin) % (uint64)PGSIZE != 0 || (page_node_t *)page == ptr) { // 非法 page
        spinlock_release(&region->lk);
        assert(false , (char *)"Invalid Page Address");
        return;
    }
    
    if (ptr == NULL) { // 空闲链表初始为空
        region->list_head.next = (page_node_t *)page;
        region->list_head.next->next = NULL;
    }
    else if (page < (uint64)ptr) { // 作为头节点插入
        region->list_head.next = (page_node_t *)page;
        region->list_head.next->next = ptr;
    }
    else {
        while (ptr->next != NULL && (uint64)ptr->next <= page)
            ptr = ptr->next;

        if ((page_node_t *)page == ptr) {   // 链表中已有这个页，重复 release
            spinlock_release(&region->lk);
            assert(false , (char *)"Invalid Page Address");
            return;
        }

        if (ptr->next == NULL) { // 作为尾节点插入
            ptr->next = (page_node_t *)page;
            ptr->next->next = NULL;
        }
        else {
            page_node_t *ptr_tmp = ptr->next;
            ptr->next = (page_node_t *)page;
            ptr->next->next = ptr_tmp;
        }
    }

    region->allocable ++;
    spinlock_release(&region->lk);
}