#ifndef MM_SLAB_H
#define MM_SLAB_H

#include <common/types.h>
#include <common/list.h>
#include <common/lock.h>
#include <mm/buddy.h>
#include <mm/page_table.h>

#define MIN_SLAB_BLOCK_ORDER 6 // 最小的slab块大小为64字节
#define MAX_SLAB_BLOCK_ORDER 11 // 最大的slab块大小为2048字节
#define SLAB_PAGE_ORDER 5
#define SLAB_SIZE ((1UL) << (SLAB_PAGE_ORDER + PAGE_SHIFT)) // slab的大小为128KB
#define SLAB_PAGES_COUNT (BUDDY_CHUNK_PAGES_COUNT(SLAB_PAGE_ORDER))
#define SLAB_POOL_SIZE (MAX_SLAB_BLOCK_ORDER - MIN_SLAB_BLOCK_ORDER + 1)

#define slab_order_to_size(order) ((1UL) << (order))
#define size_to_slab_order(size)                                                        \
	({                                                                              \
		int __order = MIN_SLAB_BLOCK_ORDER;                                     \
		if ((size) <= 0 || (size) > slab_order_to_size(MAX_SLAB_BLOCK_ORDER)) { \
			__order = -1;                                                   \
		} else {                                                                \
			while ((size) > slab_order_to_size(__order)) {                  \
				__order++;                                              \
			}                                                               \
		}                                                                       \
		__order;                                                                \
	})

#define slab_container(order) (&slab_pool_g[order - MIN_SLAB_BLOCK_ORDER])

#define for_each_container_in_slab_pool(order, sc) \
	for (order = MIN_SLAB_BLOCK_ORDER, sc = slab_container(order); order <= MAX_SLAB_BLOCK_ORDER; order++, sc++)

struct slab_header {
	int order;

	void *next_free_block;
	struct list_head partial_list_node;

	unsigned int free_block_count;
	unsigned int total_block_count;
};

struct slab_next_block {
	void *next;
};

struct slab_container {
	struct slab_header *current;
	struct list_head partial_list;
	struct lock lock;
};

void init_slab();

int slab_free(void *addr);
void *slab_alloc(size_t size);
void print_slab_info(void);

int test_slab();
#endif
