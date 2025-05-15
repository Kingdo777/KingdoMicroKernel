#ifndef MM_MM_H
#define MM_MM_H

#include <arch/mmu.h>
#include <common/lock.h>
#include <common/list.h>
#include <mm/page_table.h>
#include <mm/buddy.h>
#include <mm/slab.h>

#define PAGE_SIZE (1UL << PAGE_SHIFT)

/* Execute once during kernel init. */
void mm_init(void *physmem_info);

/* Return the size of free memory in the buddy and slab allocator. */
unsigned long get_free_mem_size(void);
unsigned long get_total_mem_size(void);

#endif /* MM_MM_H */