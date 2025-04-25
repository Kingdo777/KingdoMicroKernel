#include <mm/mm.h>
#include <mm/page_table.h>

struct mem_region memory_region_g = {0};

void mm_init(void *physmem_info)
{
        /* 1. 初始化伙伴系统 */
        init_buddy();
        

        /* Step-3: init the slab allocator. */
        // init_slab();
}