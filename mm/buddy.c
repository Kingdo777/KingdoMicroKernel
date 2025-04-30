#include <arch/boot.h>
#include <arch/mmu.h>
#include <common/macro.h>
#include <common/kprint.h>
#include <common/utils.h>
#include <mm/buddy.h>
#include <mm/mm.h>

/*
 *
 * The available memory area is represented by the diagram below.
 *
 *   |----------------|------------|--------------|
 *   | Kernel Image   |  page_arr  |   kmallooc   |
 *   |----------------|------------|--------------|
 *   0             img_end                    0x3F000000
 *
 */

static bool page_in_list(struct page *page, struct list_head *list)
{
	if (page == NULL || list == NULL || list_empty(list)) {
		return false;
	}
	struct page *tmp = NULL;
	for_each_in_list(tmp, struct page, node, list) {
		if (tmp == page) {
			return true;
		}
	}
	return false;
}

static struct page *get_buddy_chunk(struct page *chunk)
{
	if (chunk == NULL || chunk->order == BUDDY_MAX_ORDER - 1) {
		/* 如果chunk为空或已经是最大阶，返回NULL */
		return NULL;
	}
	struct page *chunk_merge = virt_to_page(
		(void *)ROUND_DOWN((unsigned long)page_to_virt(chunk),
				   BUDDY_CHUNK_SIZE(chunk->order + 1)));
	return chunk_merge == chunk ?
		       chunk + BUDDY_CHUNK_PAGES_COUNT(chunk->order) :
		       chunk_merge;
}

/*
 * @order: 要分割的阶数，范围在[1, BUDDY_MAX_ORDER - 1]
 * @return: 如果分割成功，返回其中一个chunk的指针，否则返回NULL
 */
static struct page *split_chunk(int order)
{
	struct page *chunk = NULL;
	struct page *chunk_buddy = NULL;

	/* 如果分割的阶数不在范围内，返回false */
	if (order < 1 || order >= BUDDY_MAX_ORDER) {
		kwarn("order %d is out of range\n", order);
		goto faild;
	}

	/* 判断该阶中是否存在可分割的chunk */
	struct free_list *free_list = &memory_region_g.free_lists[order];
	if (free_list->nr_free == 0) {
		/* 如果该阶中没有可分割的chunk，尝试分割更高阶的chunk */
		chunk = split_chunk(order + 1);
		if (chunk == NULL) {
			/* 如果更高阶的chunk也没有可分割的chunk，返回false */
			goto faild;
		}
	} else {
		/* 如果该阶中有可分割的chunk，直接从free_list中取出 */
		chunk = list_entry(free_list->free_list.next, struct page,
				   node);
		BUG_ON(chunk == NULL);
		list_del(&chunk->node);
		free_list->nr_free--;
	}

	/* 将该chunk分割成两个更小的chunk, 一个加入到下一阶free_list中，一个返回 */
	order--;
	free_list = &memory_region_g.free_lists[order];
	chunk_buddy = chunk + BUDDY_CHUNK_PAGES_COUNT(order);
	chunk->order = order;
	chunk_buddy->order = order;
	list_add(&chunk_buddy->node, &free_list->free_list);
	free_list->nr_free += 1;

faild:
	return chunk;
}

/*
 * @chunk: 释放的页的指针
 * @return: 如果合并成功，返回合并后的chunk的指针，否则返回传入的chunk的指针
 */
static struct page *merge_chunk(struct page *chunk_origin)
{
	struct page *chunk_merge = NULL;
	struct page *chunk_buddy = NULL;
	int order = 0;
	struct free_list *free_list = NULL;

	BUG_ON(chunk_origin == NULL);
	order = chunk_origin->order;
	free_list = &memory_region_g.free_lists[order];
	chunk_merge = chunk_origin;

	if (free_list->nr_free == 0 || order == BUDDY_MAX_ORDER - 1) {
		/* 如果该阶中没有可合并的chunk，直接返回 */
		goto no_merge;
	}

	/* 找到buddy chunk，如果不存在或已分配直接返回，否则合并 */
	chunk_merge = virt_to_page(
		(void *)ROUND_DOWN((unsigned long)page_to_virt(chunk_origin),
				   BUDDY_CHUNK_SIZE(order + 1)));
	chunk_buddy = chunk_merge == chunk_origin ?
			      chunk_origin + BUDDY_CHUNK_PAGES_COUNT(order) :
			      chunk_merge;

	if (chunk_buddy == NULL || chunk_buddy->allocated == 1 ||
	    chunk_buddy->order != order) {
		chunk_merge = chunk_origin;
		goto no_merge;
	}

	/* 这是个耗时的检查，正常不需要 */
	BUG_ON(!page_in_list(chunk_buddy, &free_list->free_list));

	/* 合并两个chunk */
	list_del(&chunk_buddy->node);
	free_list->nr_free--;

	chunk_merge->order = order + 1;

	chunk_merge = merge_chunk(chunk_merge);

no_merge:
	return chunk_merge;
}

/* 先填充中间的整块（即2MB对齐的块），再填充两头的散块 */
static void populate_page(struct free_list *free_lists, struct page *first_page,
			  unsigned long npages)
{
	struct page *page, *first_aligned_page;
	int order;

	/* 首先找到第一个与最大buddy chunk对其的页 */
	first_aligned_page = virt_to_page(
		(void *)ROUND_UP((unsigned long)page_to_virt(first_page),
				 BUDDY_CHUNK_SIZE(BUDDY_MAX_ORDER - 1)));

	/* 如果对齐页已经超出区域，直接从最小阶开始 */
	if (first_aligned_page >= first_page + npages)
		first_aligned_page = first_page;

	/* 先向后填充 [first_aligned_page, first_page + npages) */
	for (order = BUDDY_MAX_ORDER - 1, page = first_aligned_page; order >= 0;
	     order--) {
		int pages_count = BUDDY_CHUNK_PAGES_COUNT(order);
		for (; page + pages_count - 1 < first_page + npages;
		     page += pages_count) {
			page->order = order;
			page->allocated = 0;
			page->slab = NULL;
			free_lists[order].nr_free++;
			list_add(&(page->node), &(free_lists[order].free_list));
		}
	}

	/* 再向前填充 [first_page, first_aligned_page) */
	for (order = BUDDY_MAX_ORDER - 2, page = first_aligned_page; order >= 0;
	     order--) {
		int pages_count = BUDDY_CHUNK_PAGES_COUNT(order);
		for (; page - pages_count >= first_page;) {
			page -= pages_count;
			page->order = order;
			page->allocated = 0;
			page->slab = NULL;
			free_lists[order].nr_free++;
			list_add(&(page->node), &(free_lists[order].free_list));
		}
	}
}

void init_buddy()
{
	paddr_t available_phys_mem_start = 0;
	paddr_t available_phys_mem_end = 0;
	paddr_t free_phys_mem_start = 0;
	unsigned long npages = 0;
	struct page *page = NULL;

	available_phys_mem_start = ROUND_UP((paddr_t)&img_end, PAGE_SIZE);
	available_phys_mem_end = ROUND_DOWN(PHYS_MEM_END, PAGE_SIZE);
	kdebug("available pyhsical memory region: [0x%lx, 0x%lx)\n",
	       available_phys_mem_start, available_phys_mem_end);

	/* 计算包含页的数量，需要注意的是，每个页都需要一个struct page，这部分不能被分配 */
	npages = (available_phys_mem_end - available_phys_mem_start) /
		 (PAGE_SIZE + sizeof(struct page));
	/* 真正可用于分配的物理内存地址 */
	free_phys_mem_start = ROUND_UP(available_phys_mem_start +
					       npages * sizeof(struct page),
				       PAGE_SIZE);
	npages = (available_phys_mem_end - free_phys_mem_start) / PAGE_SIZE;

	/* 初始化memory_region_g，注意物理地址和虚拟地址转化 */
	memory_region_g.start_addr = (void *)phys_to_virt(free_phys_mem_start);
	memory_region_g.size = available_phys_mem_end - free_phys_mem_start;
	memory_region_g.page_arrry =
		(struct page *)phys_to_virt(available_phys_mem_start);
	memory_region_g.page_num = npages;
	kdebug("free physical memory region: [0x%lx, 0x%lx)\n",
	       free_phys_mem_start, available_phys_mem_end);

	/* 初始化 free_lists */
	lock_init(&memory_region_g.free_lists_lock);
	for (int order = 0; order < BUDDY_MAX_ORDER; ++order) {
		memory_region_g.free_lists[order].nr_free = 0;
		init_list_head(&(memory_region_g.free_lists[order].free_list));
	}

	/* 初始化每个页 */
	memset((char *)memory_region_g.page_arrry, 0,
	       npages * sizeof(struct page));

	populate_page(memory_region_g.free_lists, memory_region_g.page_arrry,
		      npages);
}

void buddy_free_pages(struct page *page)
{
	struct page *merge_page = NULL;
	int order = 0;
	struct free_list *free_list = NULL;

	lock(&memory_region_g.free_lists_lock);
	page->allocated = 0;
	merge_page = merge_chunk(page);
	order = merge_page->order;
	free_list = &memory_region_g.free_lists[order];
	list_add(&merge_page->node, &free_list->free_list);
	free_list->nr_free++;
	unlock(&memory_region_g.free_lists_lock);
}

struct page *buddy_get_pages(int order)
{
	struct page *page = NULL;
	struct free_list *free_list = NULL;

	lock(&memory_region_g.free_lists_lock);

	free_list = &memory_region_g.free_lists[order];
	if (free_list->nr_free == 0) {
		page = split_chunk(order + 1);
		if (page == NULL) {
			goto no_page;
		}
	} else {
		page = list_entry(free_list->free_list.next, struct page, node);
		BUG_ON(page == NULL);
		list_del(&page->node);
		free_list->nr_free--;
	}
	if (page->allocated == 1) {
		/* 如果该页已经被分配，说明有bug */
		// TODO_DELETE
		kerror("page %p has been allocated\n", page);
		goto no_page;
	}
	BUG_ON(page->allocated == 1);
	BUG_ON(page->order != order);
	page->allocated = 1;

no_page:
	unlock(&memory_region_g.free_lists_lock);

	return page;
}

void *page_to_virt(struct page *page)
{
	return (void *)(memory_region_g.start_addr +
			page_to_pfn(page) * PAGE_SIZE);
}

struct page *virt_to_page(void *ptr)
{
	if (ptr < memory_region_g.start_addr ||
	    ptr >= memory_region_g.start_addr + memory_region_g.size) {
		return NULL;
	}

	unsigned long pfn =
		(unsigned long)(ptr - memory_region_g.start_addr) / PAGE_SIZE;
	if (pfn >= memory_region_g.page_num) {
		return NULL;
	}
	return pfn_to_page(pfn);
}

unsigned long get_free_pages_nums_from_buddy()
{
	int order;
	unsigned long total_size = 0;
	for (order = 0; order < BUDDY_MAX_ORDER; ++order) {
		total_size += memory_region_g.free_lists[order].nr_free *
			      BUDDY_CHUNK_PAGES_COUNT(order);
	}
	return total_size;
}

unsigned long get_free_mem_size_from_buddy()
{
	int order;
	unsigned long total_size = 0;
	for (order = 0; order < BUDDY_MAX_ORDER; ++order) {
		total_size += memory_region_g.free_lists[order].nr_free *
			      BUDDY_CHUNK_SIZE(order);
	}
	return total_size;
}

unsigned long get_total_mem_size_from_buddy()
{
	return memory_region_g.size;
}

void print_buddy_info()
{
	kinfo("Free pages has %ld: \n", get_free_pages_nums_from_buddy());
	for (int order = 0; order < BUDDY_MAX_ORDER; ++order) {
		kinfo("order %d: %ld\n", order,
		      memory_region_g.free_lists[order].nr_free);
	}
	kinfo("Free Memory size: %ldMB %ldKB, Total Memory size: %ldMB %ldKB\n",
	      get_free_mem_size_from_buddy() / 1024 / 1024,
	      get_free_mem_size_from_buddy() / 1024 % 1024,
	      get_total_mem_size_from_buddy() / 1024 / 1024,
	      get_total_mem_size_from_buddy() / 1024 % 1024);
}