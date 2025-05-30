#ifndef MM_KMALLOC_H
#define MM_KMALLOC_H

#include <common/types.h>
#include <mm/slab.h>
#include <mm/buddy.h>

void *get_pages(int order);
void free_pages(void *addr);

void *kmalloc(size_t size);
void *kzalloc(size_t size);

void kfree(void *ptr);
void kmalloc_test();

#endif