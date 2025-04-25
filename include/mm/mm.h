#ifndef MM_MM_H
#define MM_MM_H

#include <common/lock.h>
#include <common/list.h>
#include <mm/page_table.h>
#include <mm/buddy.h>

/* Execute once during kernel init. */
void mm_init(void* physmem_info);

#endif /* MM_MM_H */