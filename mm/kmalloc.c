#include <mm/kmalloc.h>
#include <mm/slab.h>
#include <mm/buddy.h>
#include <common/utils.h>

#define ZERO_SIZE_PTR ((void *)(-1UL))
#define IS_VALID_PTR(ptr) ((ptr) != NULL && (ptr) != ZERO_SIZE_PTR)

void *get_pages(int order)
{
	struct page *page = NULL;
	void *addr;

	page = buddy_get_pages(order);

	if (unlikely(!page)) {
		kwarn("[OOM] Cannot get page from Buddy!\n");
		return NULL;
	}

	addr = page_to_virt(page);

	return addr;
}

void free_pages(void *addr)
{
	struct page *page;

	page = virt_to_page(addr);
	if (!page) {
		kdebug("invalid page in %s", __func__);
		return;
	}

	buddy_free_pages(page);
}

void *__kmalloc(size_t size, size_t *real_size)
{
	struct page *page;
	void *addr;

	if (size <= slab_order_to_size(MAX_SLAB_BLOCK_ORDER)) {
		*real_size = slab_order_to_size(size_to_slab_order(size));
		return slab_alloc(size);
	} else if (size <= BUDDY_CHUNK_SIZE(BUDDY_MAX_ORDER - 1)) {
		*real_size = BUDDY_CHUNK_SIZE(size_to_page_order(size));
		return get_pages(size_to_page_order(size));
	} else {
		kwarn("kmalloc size %zu is too large\n", size);
		return NULL;
	}
}

void *kmalloc(size_t size)
{
	size_t real_size;
	void *addr = NULL;
	if (size == 0) {
		addr = ZERO_SIZE_PTR;
	} else {
		addr = __kmalloc(size, &real_size);
	}
	return addr;
}

void *kzalloc(size_t size)
{
	void *addr;

	addr = kmalloc(size);
	if (!IS_VALID_PTR(addr)) {
		memset(addr, 0, size);
	}
	return addr;
}

void kfree(void *ptr)
{
	struct page *page;

	if (unlikely(ptr == ZERO_SIZE_PTR))
		return;

	page = virt_to_page(ptr);

	if (!page) {
		kwarn("ptr %p is not a valid pointer\n", ptr);
		return;
	}

	if (page->slab) {
		slab_free(ptr);
	} else if (page) {
		free_pages(ptr);
	} else {
		kwarn("unexpected state in %s\n", __func__);
	}
}

void kmalloc_test()
{
	size_t free_buddy_size_before = get_free_mem_size_from_buddy();
	int loop = 1000;
	while (loop--) {
		for (int i = 1; i < 23; i++) {
			size_t size = (1UL << i) - 1;
			size_t size_expt = 1UL << i;
			size_t size_real;
			void *ptr = __kmalloc(size, &size_real);
			// kinfo("kmalloc %u, real size %u ,ptr %p\n", size,
			//       size_real, ptr);
			kfree(ptr);
		}
	}
	size_t free_buddy_size_after = get_free_mem_size_from_buddy();
	assert(free_buddy_size_after == free_buddy_size_before); // 确保释放的内存和分配的内存一致
	kinfo("kmalloc test passed\n");
}
