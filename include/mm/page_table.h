#ifndef MM_PAGE_TABLE_H
#define MM_PAGE_TABLE_H

#include <arch/mm/page_table.h>
#include <uapi/memory.h>

void set_page_table(paddr_t pgtbl);

int map_range_in_pgtbl_kernel(void *pgtbl, vaddr_t va, paddr_t pa, size_t len, vmr_prop_t flags);

int map_range_in_pgtbl_user(void *pgtbl, vaddr_t va, paddr_t pa, size_t len, vmr_prop_t flags, long *rss);

#endif