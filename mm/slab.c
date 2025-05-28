#include <mm/slab.h>
#include <common/kprint.h>
#include <common/errno.h>
#include <common/lock.h>

struct slab_container slab_pool_g[SLAB_POOL_SIZE] = { 0 };

static bool container_is_free_enough(struct slab_container *sc)
{
	// 如果当前slab的空闲块数大于等于slab总块数的一半，则认为该slab足够空闲
	// 这里的判断条件是为了避免频繁地从伙伴系统中申请新的slab
	// 当然，这个条件可以根据实际情况进行调整
	BUG_ON(sc == NULL);
	if (sc->current == NULL || sc->current->free_block_count < sc->current->total_block_count / 2) {
		return false;
	}
	return true;
}

static int alloc_slab_from_buddy(int order)
{
	int error = 0;
	struct page *page = NULL;
	struct slab_header *s = NULL;
	struct slab_container *container = NULL;
	void *block = NULL;

	container = slab_container(order);

	page = buddy_get_pages(SLAB_PAGE_ORDER);
	if (page == NULL) {
		return -ENOMEM;
	}

	s = (struct slab_header *)page_to_virt(page);
	s->order = order;
	s->total_block_count = SLAB_SIZE / slab_order_to_size(order);
	s->free_block_count = s->total_block_count - 1;

	container->current = s;
	block = ((void *)s + slab_order_to_size(order));
	container->current->next_free_block = block;

	while (block < (void *)s + SLAB_SIZE - slab_order_to_size(order)) {
		void *next_block = block + slab_order_to_size(order);
		((struct slab_next_block *)block)->next = next_block;
		block = next_block;
	}
	((struct slab_next_block *)(block))->next = NULL;

	for (int i = 0; i < SLAB_PAGES_COUNT; i++, page++) {
		page->slab = (void *)s;
	}

	return error;
}

static void *slab_get_free_block(struct slab_header *s)
{
	void *block = NULL;

	BUG_ON(!s->free_block_count);
	BUG_ON(!s->next_free_block);

	block = s->next_free_block;
	s->free_block_count--;
	s->next_free_block = ((struct slab_next_block *)block)->next;

	return block;
}

static struct slab_header *get_from_partial_list(struct slab_container *sc)
{
	struct slab_header *s = NULL;
	if (list_empty(&sc->partial_list)) {
		s = NULL;
	} else {
		s = list_first_entry(&sc->partial_list, struct slab_header, partial_list_node);
		list_del(&s->partial_list_node);
	}
	return s;
}

void set_new_current_slab(struct slab_container *sc, int order)
{
	int err = 0;
	BUG_ON(sc == NULL);

	sc->current = get_from_partial_list(sc);
	if (sc->current == NULL) {
		err = alloc_slab_from_buddy(order);
		if (err != 0) {
			sc->current = NULL;
			kwarn("OOM!!! slab_alloc: alloc slab from buddy failed\n");
		}
	}
}

static void *__slab_alloc(int order)
{
	int err = 0;
	void *addr = NULL;
	struct slab_container *sc = NULL;
	BUG_ON(order < MIN_SLAB_BLOCK_ORDER || order > MAX_SLAB_BLOCK_ORDER);

	sc = slab_container(order);
	lock(&sc->lock);

	if (sc->current == NULL) {
		err = alloc_slab_from_buddy(order);
		if (err != 0) {
			kwarn("OOM!!! slab_alloc: alloc slab from buddy failed\n");
			goto failed;
		}
	}

	addr = slab_get_free_block(sc->current);

	if (sc->current->free_block_count == 0) {
		BUG_ON(sc->current->next_free_block != NULL);
		set_new_current_slab(sc, order);
	}

failed:
	unlock(&sc->lock);
	return addr;
}

static int put_slab_to_buddy(struct slab_header *s)
{
	struct page *page = NULL;

	page = virt_to_page((void *)s);
	if (page == NULL) {
		kerror("put_slab_to_buddy: invalid slab header %p\n", s);
		return -EINVAL;
	}
	if (s->free_block_count != s->total_block_count - 1) {
		kerror("put_slab_to_buddy: slab header %p is not full free\n", s);
		return -EINVAL;
	}

	for (int i = 0; i < SLAB_PAGES_COUNT; i++) {
		(page + i)->slab = NULL;
	}

	buddy_free_pages(page);

	return 0;
}

static void slab_put_block(struct slab_header *s, void *block)
{
	unsigned long block_index = 0;

	BUG_ON(s == NULL);
	BUG_ON(block == NULL);

	block_index = ((unsigned long)block - (unsigned long)s) / slab_order_to_size(s->order);
	block = (void *)((unsigned long)s + block_index * slab_order_to_size(s->order));

	((struct slab_next_block *)block)->next = s->next_free_block;
	s->next_free_block = block;
	s->free_block_count++;
}

int __slab_free(struct slab_header *s, void *block)
{
	int err = 0;
	int order = 0;
	struct slab_container *sc = NULL;

	order = s->order;
	BUG_ON(order < MIN_SLAB_BLOCK_ORDER || order > MAX_SLAB_BLOCK_ORDER);
	sc = slab_container(order);

	lock(&sc->lock);

	slab_put_block(s, block);

	if (s->free_block_count == 1) {
		// 如果之前slab的空闲块数为0，归还一个块后将其加入到partial_list的末尾
		list_append(&s->partial_list_node, &sc->partial_list);
		BUG_ON(sc->current == s);
	} else if (s->free_block_count == s->total_block_count - 1) {
		// 如果释放块后slab完全空闲，则归还给伙伴系统
		err = put_slab_to_buddy(s);
		if (err == 0) {
			if (sc->current == s) {
				sc->current = get_from_partial_list(sc);
			} else {
				list_del(&s->partial_list_node);
			}
		} else {
			kerror("slab_free: put slab to buddy failed\n");
		}
	}

	unlock(&sc->lock);

	return err;
}

static bool is_aligned(void *addr, int order)
{
	return ((unsigned long)addr & (slab_order_to_size(order) - 1)) == 0;
}

static struct slab_header *slab_get_header(void *addr)
{
	struct slab_header *s = NULL;
	struct page *page = NULL;

	page = virt_to_page(addr);
	if (page == NULL || page->slab == NULL) {
		kerror("slab_get_header: invalid address %p\n", addr);
		return NULL;
	}

	s = (struct slab_header *)page->slab;

	return s;
}

void init_slab()
{
	int order = 0;
	struct slab_container *sc = NULL;

	for_each_container_in_slab_pool(order, sc) {
		sc->current = NULL;
		init_list_head(&sc->partial_list);
		lock_init(&sc->lock);
	}
}

void *slab_alloc(size_t size)
{
	int order;

	order = size_to_slab_order(size);
	if (order == -1) {
		kwarn("slab_alloc: invalid size %u\n", size);
		return NULL;
	}

	return __slab_alloc(order);
}

int slab_free(void *addr)
{
	int err = 0;
	struct slab_header *s = NULL;

	if (addr == NULL) {
		kwarn("slab_free: address is NULL\n");
		err = -EINVAL;
	}

	if (err == 0) {
		if (!is_aligned(addr, MIN_SLAB_BLOCK_ORDER)) {
			kerror("slab_free: address %p is not aligned\n", addr);
			err = -EINVAL;
		}
	}

	if (err == 0) {
		s = slab_get_header(addr);
		if (s != NULL) {
			err = __slab_free(s, addr);
			if (err != 0) {
				kerror("slab_free: failed to free address %p\n", addr);
			}
		}
	}

	return err;
}

void calculate_slab_free_block_count(int counts[SLAB_POOL_SIZE])
{
	int order = 0;
	struct slab_container *sc = NULL;

	for_each_container_in_slab_pool(order, sc) {
		struct slab_header *s = NULL;
		unsigned long free_block_count = 0;

		free_block_count = sc->current ? sc->current->free_block_count : 0;
		for_each_in_list(s, struct slab_header, partial_list_node, &sc->partial_list) {
			free_block_count += s->free_block_count;
		}
		counts[order - MIN_SLAB_BLOCK_ORDER] = free_block_count;
	}
}

void print_slab_info(void)
{
	int counts[SLAB_POOL_SIZE] = { 0 };
	calculate_slab_free_block_count(counts);
	for (int order = 0; order <= MAX_SLAB_BLOCK_ORDER - MIN_SLAB_BLOCK_ORDER; order++) {
		kinfo("slab pool of order-%d: free block count %lu\n", order, counts[order]);
	}
}
