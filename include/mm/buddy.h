#ifndef MM_BUDDY_H
#define MM_BUDDY_H

#include <common/types.h>
#include <common/list.h>
#include <common/lock.h>

/* 和 Linux 保持一致，最大可分配连续物理内存为4MB */
#define BUDDY_MAX_ORDER (11)
#define BUDDY_CHUNK_SIZE(order) ((1UL) << (PAGE_SHIFT + order))
#define BUDDY_CHUNK_SIZE_MASK(order) (BUDDY_CHUNK_SIZE(order) - 1)
#define BUDDY_CHUNK_PAGES_COUNT(order) ((1UL) << order)

#define size_to_page_order(size)                                                     \
	({                                                                           \
		int __order = 0;                                                     \
		if ((size) <= 0 || (size) > BUDDY_CHUNK_SIZE(BUDDY_MAX_ORDER - 1)) { \
			__order = -1;                                                \
		} else {                                                             \
			while ((size) > BUDDY_CHUNK_SIZE(__order)) {                 \
				__order++;                                           \
			}                                                            \
		}                                                                    \
		__order;                                                             \
	})

#define pfn_to_page(pfn) (memory_region_g.page_arrry + pfn)
#define page_to_pfn(page) ((unsigned long)((page) - memory_region_g.page_arrry))

extern struct mem_region memory_region_g;

/*
 * 我们采用平坦模型来管理所有的物理内存，即整个物理内存页的虚拟地址和物理地址都是连续的。
 * 我们将page 数组存放在img_end 之后的内存中，物理内存的起始地址为img_end。因为raspi3的
 * 可用物理内存也是一整块。关于内存模型的更多信息请参考：https://zhuanlan.zhihu.com/p/503695273
*/

struct free_list {
	struct list_head free_list;
	unsigned long nr_free;
};

struct mem_region {
	/* 可分配物理内存的起始地址，不包括存放page 数组的内存 */
	void *start_addr;
	/* 可分配物理内存的大小 */
	size_t size;

	/* page 数组的起始地址 */
	struct page *page_arrry;
	/* page 数量 */
	size_t page_num;

	/* 内存分配释放锁 */
	struct lock free_lists_lock;
	struct free_list free_lists[BUDDY_MAX_ORDER];
};

/* `struct page` is the metadata of one physical 4k page. */
struct page {
	struct list_head node; /* Free list */
	int allocated;
	int order;
	void *slab;
};

void init_buddy();
struct page *buddy_get_pages(int order);
void buddy_free_pages(struct page *page);

void *page_to_virt(struct page *page);
struct page *virt_to_page(void *ptr);

unsigned long get_free_mem_size_from_buddy();
unsigned long get_total_mem_size_from_buddy();
unsigned long get_free_pages_nums_from_buddy();
void print_buddy_info();

#endif /* MM_BUDDY_H */
