#include <common/utils.h>
#include <mm/kmalloc.h>
#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>
#include <lib/printk.h>
// #include <mm/vmspace.h>
#include <mm/mm.h>
#include <mm/common_pte.h>
#include <arch/mmu.h>
#include <arch/sync.h>

#include <arch/mm/page_table.h>

/**
 * @brief: 设置用户页表基址寄存器，用于切换用户态进程的页表
 * @param pgtbl: 页表物理地址（注意不是虚拟地址）
*/
void set_page_table(paddr_t pgtbl)
{
	set_ttbr0_el1(pgtbl);
}

static int __vmr_prot_to_ap(vmr_prop_t prot)
{
	if ((prot & VMR_READ) && !(prot & VMR_WRITE)) {
		return AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;
	} else if (prot & VMR_WRITE) {
		return AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
	}
	return 0;
}

static int __ap_to_vmr_prot(int ap)
{
	if (ap == AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO) {
		return VMR_READ;
	} else if (ap == AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW) {
		return VMR_READ | VMR_WRITE;
	}
	return 0;
}

#define USER_PTE 0
#define KERNEL_PTE 1

/**
 * @brief: 设置页表项的属性
 * @param entry: 页表项
 * @param flags: 映射属性
 * @param kind: 映射类型（内核/用户）
 * @return: 0 on success, -1 on error
*/
static int set_pte_flags(pte_t *entry, vmr_prop_t flags, int kind)
{
	BUG_ON(kind != USER_PTE && kind != KERNEL_PTE);

	/**
	 * AP[1:0] 负责设置el1和el0的访问权限
	 * 当前访问权限（AP）设置，映射的页面始终是可读的（不考虑XOM，Execute-Only Memory
	 * EL1可以直接访问EL0（显式禁用SMAP，Supervisor Mode Access Prevention）
	*/
	entry->l3_page.AP = __vmr_prot_to_ap(flags);

	/**
	 * 内核态不能直接执行用户态的代码，用户态也不能直接执行内核态的代码
	*/
	if (kind == KERNEL_PTE) { // kernel PTE
		// 如果没有执行权限，则设置PXN位，特权态也无法执行该页
		if (!(flags & VMR_EXEC))
			entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PXN;
		// 设置UXN（Unprivileged eXecute-Never）位，非特权态无法执行
		entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;
	} else { // User PTE
		// 如果没有执行权限，则设置UXN位，用户态不能执行该页
		if (!(flags & VMR_EXEC))
			entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;
		// 设置PXN（Privileged eXecute-Never）位，特权态不能直接执行该页
		entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PXN;
	}

	// 设置访问标志位
	entry->l3_page.AF = AARCH64_MMU_ATTR_PAGE_AF_ACCESSED;
	// 设置非全局标志位，表示该页表项在TLB中的缓存只对当前ASID有效，读于实现进程隔离很重要
	entry->l3_page.nG = 1;
	// 定义共享属性，INNER_SHAREABLE表示被"内层域"（如CPU核组）共享，需维护所有缓存一致性（L1/L2/L3）
	entry->l3_page.SH = INNER_SHAREABLE;
	// 设置内存类型
	if (flags & VMR_DEVICE) { // 设备内存
		entry->l3_page.attr_index = DEVICE_MEMORY;
		entry->l3_page.SH = 0;
	} else if (flags &
		   VMR_NOCACHE) { // 非缓存内存，CPU 访问该内存时绕过缓存，直接读写物理内存
		entry->l3_page.attr_index = NORMAL_MEMORY_NOCACHE;
	} else { // 普通内存
		entry->l3_page.attr_index = NORMAL_MEMORY;
	}

	return 0;
}

#define GET_PADDR_IN_PTE(entry) \
	(((u64)(entry)->table.next_table_addr) << PAGE_SHIFT)
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define NORMAL_PTP (0)
#define BLOCK_PTP (1)

/**
 * @brief: 获取给定虚拟地址的下一级页表页
 * @param cur_ptp: 当前页表页的虚拟地址
 * @param level: 当前页表页的级别（L0-L3）
 * @param va: 虚拟地址
 * @param next_ptp: 返回的下一级页表的虚拟地址
 * @param pte: 返回的当前页表页中对应的页表项
 * @param alloc: 是否分配新的页表页
 * @param rss: 映射的物理页数
 * @return: 返回下一级页表的类型，如果是最后一级页表或者块页表，则返回BLOCK_PTP，
 *         否则返回NORMAL_PTP
*/
static int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va, ptp_t **next_ptp,
			pte_t **pte, bool alloc, long *rss)
{
	u32 index = 0;
	pte_t *entry;

	if (cur_ptp == NULL)
		return -ENOMEM;

	switch (level) {
	case L0:
		index = GET_L0_INDEX(va);
		break;
	case L1:
		index = GET_L1_INDEX(va);
		break;
	case L2:
		index = GET_L2_INDEX(va);
		break;
	case L3:
		index = GET_L3_INDEX(va);
		break;
	default:
		BUG("unexpected level\n");
		return -EINVAL;
	}

	entry = &(cur_ptp->ent[index]);
	if (IS_PTE_INVALID(entry->pte)) {
		if (alloc == false) {
			return -ENOMEM;
		} else {
			/* alloc a new page table page */
			ptp_t *new_ptp;
			paddr_t new_ptp_paddr;
			pte_t new_pte_val;

			new_ptp = get_pages(0);
			if (new_ptp == NULL)
				return -ENOMEM;
			memset((void *)new_ptp, 0, PAGE_SIZE);
			if (rss)
				*rss += PAGE_SIZE;

			new_ptp_paddr = virt_to_phys((vaddr_t)new_ptp);

			new_pte_val.pte = 0;
			new_pte_val.table.is_valid = 1;
			new_pte_val.table.is_table = 1;
			new_pte_val.table.next_table_addr = new_ptp_paddr >>
							    PAGE_SHIFT;

			/* same effect as: cur_ptp->ent[index] = new_pte_val; */
			entry->pte = new_pte_val.pte;
		}
	}

	*next_ptp = (ptp_t *)GET_NEXT_PTP(entry);
	*pte = entry;
	if (IS_PTE_TABLE(entry->pte))
		return NORMAL_PTP;
	else
		return BLOCK_PTP;
}

/**
 * @brief: 查询页表中虚拟地址va对应的物理地址pa
 * @param pgtbl: 页表基址（虚拟地址）
 * @param va: 虚拟地址
 * @param pa: 返回的物理地址
 * @param entry: 返回的页表项
*/
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
	/* On aarch64, l0 is the highest level page table */
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	ptp_t *phys_page;
	pte_t *pte;
	int ret;

	// L0 page table
	l0_ptp = (ptp_t *)pgtbl;
	ret = get_next_ptp(l0_ptp, L0, va, &l1_ptp, &pte, false, NULL);
	if (ret < 0)
		return ret;

	// L1 page table
	ret = get_next_ptp(l1_ptp, L1, va, &l2_ptp, &pte, false, NULL);
	if (ret < 0)
		return ret;
	else if (ret == BLOCK_PTP) {
		*pa = virt_to_phys((vaddr_t)l2_ptp) + GET_VA_OFFSET_L1(va);
		if (entry)
			*entry = pte;
		return 0;
	}

	// L2 page table
	ret = get_next_ptp(l2_ptp, L2, va, &l3_ptp, &pte, false, NULL);
	if (ret < 0)
		return ret;
	else if (ret == BLOCK_PTP) {
		*pa = virt_to_phys((vaddr_t)l3_ptp) + GET_VA_OFFSET_L2(va);
		if (entry)
			*entry = pte;
		return 0;
	}

	// L3 page table
	ret = get_next_ptp(l3_ptp, L3, va, &phys_page, &pte, false, NULL);
	if (ret < 0)
		return ret;
	// phys_page 指向了va对应的物理页的虚拟地址，GET_VA_OFFSET_L3(va)获取页内偏移
	*pa = virt_to_phys((vaddr_t)phys_page) + GET_VA_OFFSET_L3(va);
	if (entry)
		*entry = pte;
	return 0;
}

/**
 * @brief: 在内核或用户页表中映射物理地址到指定虚拟地址
 * @param pgtbl: 内核/用户页表基址（虚拟地址）
 * @param va: 虚拟地址
 * @param pa: 物理地址
 * @param len: 映射长度
 * @param flags: 映射属性
 * @param kind: 映射类型（内核/用户）
 * @param rss: 映射的物理页数
*/
static int map_range_in_pgtbl_common(void *pgtbl, vaddr_t va, paddr_t pa,
				     size_t len, vmr_prop_t flags, int kind,
				     long *rss)
{
	s64 total_page_cnt;
	ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
	pte_t *pte;
	int ret;
	int pte_index;
	int i;

	BUG_ON(pgtbl == NULL);
	BUG_ON(va % PAGE_SIZE);
	total_page_cnt = len / PAGE_SIZE + (((len % PAGE_SIZE) > 0) ? 1 : 0);

	// 四级页表管理中，l0_ptp是最高级页表，只占据一个页
	l0_ptp = (ptp_t *)pgtbl;
	l1_ptp = NULL;
	l2_ptp = NULL;
	l3_ptp = NULL;

	while (total_page_cnt > 0) {
		// 通过l0_ptp获取l1_ptp，如果l1_ptp不存在，则分配一个新的l1_ptp
		ret = get_next_ptp(l0_ptp, L0, va, &l1_ptp, &pte, true, rss);
		BUG_ON(ret != 0);
		// 通过l1_ptp获取l2_ptp，如果l2_ptp不存在，则分配一个新的l2_ptp
		ret = get_next_ptp(l1_ptp, L1, va, &l2_ptp, &pte, true, rss);
		BUG_ON(ret != 0);
		// 通过l2_ptp获取l3_ptp，如果l3_ptp不存在，则分配一个新的l3_ptp
		ret = get_next_ptp(l2_ptp, L2, va, &l3_ptp, &pte, true, rss);
		BUG_ON(ret != 0);
		// 通过l3_ptp获取物理页
		pte_index = GET_L3_INDEX(va); // 计算当前页表项的索引
		for (i = pte_index; i < PTP_ENTRIES; ++i) {
			// 设置l3_ptp的页表项
			pte_t new_pte_val;
			new_pte_val.pte = 0; // 清空页表项
			new_pte_val.l3_page.is_valid = 1; // 设置有效位
			new_pte_val.l3_page.is_page = 1; // 设置页表项类型为页
			new_pte_val.l3_page.pfn = pa >>
						  PAGE_SHIFT; // 设置物理页号
			set_pte_flags(&new_pte_val, flags,
				      kind); // 设置页表项属性
			l3_ptp->ent[i].pte = new_pte_val.pte; // 更新页表项

			va += PAGE_SIZE;
			pa += PAGE_SIZE;
			if (rss)
				*rss += PAGE_SIZE;
			total_page_cnt -= 1;
			// 如果映射范围很广，可能跨越多个l3，l2，l1页表页
			if (total_page_cnt == 0)
				break;
		}
	}

	dsb(ishst); // 数据同步屏障，等待TLB、缓存等更新完成
	isb(); // 指令同步屏障，确保后续指令在更新后的状态下执行

	/* Since we are adding new mappings, there is no need to flush TLBs. */
	return 0;
}

/**
 * @brief: 在内核页表中映射物理地址到虚拟地址
 * @param pgtbl: 内核页表基址（虚拟地址）
 * @param va: 虚拟地址
 * @param pa: 物理地址
 * @param len: 映射长度
 * @param flags: 映射属性
*/
int map_range_in_pgtbl_kernel(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
			      vmr_prop_t flags)
{
	return map_range_in_pgtbl_common(pgtbl, va, pa, len, flags, KERNEL_PTE,
					 NULL);
}

/**
 * @brief: 在用户页表中映射物理地址到虚拟地址
 * @param pgtbl: 用户页表基址（虚拟地址）
 * @param va: 虚拟地址
 * @param pa: 物理地址
 * @param len: 映射长度
 * @param flags: 映射属性
*/
int map_range_in_pgtbl_user(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
			    vmr_prop_t flags, long *rss)
{
	return map_range_in_pgtbl_common(pgtbl, va, pa, len, flags, USER_PTE,
					 rss);
}
