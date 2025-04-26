#ifndef MM_MM_H
#define MM_MM_H

#include <common/lock.h>
#include <common/list.h>
#include <mm/page_table.h>
#include <mm/buddy.h>

/* Execute once during kernel init. */
void mm_init(void *physmem_info);

/* Return the size of free memory in the buddy and slab allocator. */
unsigned long get_free_mem_size(void);
unsigned long get_total_mem_size(void);

#endif /* MM_MM_H */