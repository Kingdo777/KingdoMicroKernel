#include "image.h"

typedef unsigned long u64;
typedef unsigned int u32;

/*
 * 在树莓派 3B+ 机器上，其物理内存大小为 1GB，分布如下：
 * 0x00000000~0x3f000000 物理内存（SDRAM），1008 MB
 * 0x3f000000~0x40000000 共享外设内存（Peripheral），是​​外设寄存器（MMIO）的地址映射，16 MB
 * 0x40000000~0xFFFFFFFF 本地外设内存，每个 CPU 核独立，3 GB
*/
#define PHYSMEM_START (0x0UL)
#define PERIPHERAL_BASE (0x3F000000UL)
#define SHARED_PERIPHERAL_END (0x40000000UL)
#define PHYSMEM_END (0xFFFFFFFFUL)

// PTP：页表页（Page Table Page）
#define PTP_ENTRIES 512
#define PTP_SIZE 4096
#define ALIGN(n) __attribute__((__aligned__(n)))

/*
 * 这里0x00000000 ~ 0x40000000 使用2MB粒度映射
 * 0x40000000 ~ 0xFFFFFFFF 使用1GB粒度映射
 * 因为最大内存只需要1GB，且3GB的本地外设内存使用1GB粒度映射，
 * 因此，L0/L1/L2只需要一个页表页，且不需要L3
 * ttbr0 是低地址空间的页表基地址寄存器，ttbr1 是高地址空间的页表基地址寄存器
 * 高低的标准由 tcr_el1 寄存器的t0sz和t1sz位决定
 * 一般情况下，我们会将 tcr_el1 配置为高低地址各有 48 位的地址范围，即：
 * 0x0000_0000_0000_0000～0x0000_ffff_ffff_ffff 为低地址
 * 0xffff_0000_0000_0000～0xffff_ffff_ffff_ffff 为高地址
 *
*/
u64 boot_ttbr0_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);

u64 boot_ttbr1_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);

/*
 *  bit[0]:    有效位，表示该页表项是否有效，有效表示指向下一级页表、内存页或内存块
 *  bit[1]:    表示该页表项是否指向下一级页表
 *  bits[2-4]: 表示内存属性索引，间接指向 mair_el1 寄存器中配置的属性9，用于控制将物
 *             理页映射为正常内存（normal memory）或设备内存（device memory），以及控制 cache 策略等
 *  bits[7-6]: 表示读写等数据访问权限
 *  bits[8-9]: 表示可共享属性
 *  bit[10]:   访问标志位（AF），置为 1 表示该页/块在上一次 AF 置 0 后被访问过
 *  bit[11]:   非全局标志位（NG），置为 1 表示该描述符在 TLB 中的缓存只对当前 ASID(Address Space ID) 有效
 *  bit[53]:   PXN位，置为 1 表示特权态无法执行（Privileged eXecute-Never）
 *  bit[54]:   UXN位，置为 1 表示非特权态无法执行（Unprivileged eXecute-Never）
 * 
*/
#define IS_VALID (1UL << 0) // 有效位
#define IS_TABLE (1UL << 1) // 表示该页表项指向下一级页表
#define UXN (0x1UL << 54) // 非特权态无法执行
#define ACCESSED (0x1UL << 10) // 访问标志位
#define NG (0x1UL << 11) // 非全局标志位
#define INNER_SHARABLE (0x3UL << 8) // 可共享属性为内部共享
#define NORMAL_MEMORY (0x4UL << 2) // 普通内存
#define DEVICE_MEMORY (0x0UL << 2) // 设备内存

#define SIZE_2M (2UL * 1024 * 1024)
#define SIZE_1G (1024 * 1024 * 1024)

#define GET_L0_INDEX(x) \
	(((x) >> (12 + 9 + 9 + 9)) & 0x1ff) // 提取虚拟地址的 L0 索引
#define GET_L1_INDEX(x) \
	(((x) >> (12 + 9 + 9)) & 0x1ff) // 提取虚拟地址的 L1 索引
#define GET_L2_INDEX(x) (((x) >> (12 + 9)) & 0x1ff) // 提取虚拟地址的 L2 索引
#define GET_L3_INDEX(x) (((x) >> (12)) & 0x1ff) // 提取虚拟地址的 L3 索引

void init_kernel_pt(void)
{
	u64 vaddr = PHYSMEM_START;

	/// ！！！！ 此时没有开启MMU，因此虚拟地址和物理地址是一样的 ！！！！！
	/// 这里将物理内存映射到高位，即 KERNEL_VADDR + addr(vaddr) 上
	/// 但是需要注意的是，低位也需要映射，因为当前的内核代码是运行在低位的，虚拟地址和物理地址是一样的
	/// 一旦开启了 MMU，虚拟地址和物理地址就不一样了，但是PC指针还是指向低位的，如果不映射低位，
	/// 那么就会在开启MMU后，无法访问正确的物理内存了

	/* 一、配置 boot_ttbr1，映射高位虚拟内存，即在现有内存地址的基础上添加 KERNEL_VADDR 偏移 */

	/* 1.1 L0 只需要一个页表页，即 L1 页表页的地址 */
	boot_ttbr1_l0[GET_L0_INDEX(vaddr + KERNEL_VADDR)] =
		((u64)boot_ttbr1_l1) | IS_TABLE | IS_VALID | NG;
	/* 1.2 以 2MB 页粒度映射 0x0000000 ~ 0x40000000 的内存 */
	// 首先把L2页表页的地址写入 L1 页表项
	boot_ttbr1_l1[GET_L1_INDEX(vaddr + KERNEL_VADDR)] =
		((u64)boot_ttbr1_l2) | IS_TABLE | IS_VALID | NG;
	// 每2MB一个条项，写入 L2 页表项，映射 0x00000000 ~ 0x3F000000 的普通内存
	for (; vaddr < PERIPHERAL_BASE; vaddr += SIZE_2M) {
		boot_ttbr1_l2[GET_L2_INDEX(vaddr + KERNEL_VADDR)] =
			(vaddr) | UXN /* 非特权态无法执行 */
			| ACCESSED /* 访问标志位 */
			| NG /* 非全局标志位 */
			| INNER_SHARABLE /* 内部共享 */
			| NORMAL_MEMORY /* 普通内存 */
			| IS_VALID; /* 有效位 */
	}
	// 每2MB一个条项，写入 L2 页表项，0x3F000000 ~ 0x40000000 映射为设备内存
	for (; vaddr < SHARED_PERIPHERAL_END; vaddr += SIZE_2M) {
		boot_ttbr1_l2[GET_L2_INDEX(vaddr + KERNEL_VADDR)] =
			(vaddr) | UXN | ACCESSED | NG | INNER_SHARABLE |
			DEVICE_MEMORY /* 设备内存 */
			| IS_VALID;
	}
	/* 1.3 以 1GB 页粒度映射 0x40000000~0xFFFFFFFF 的内存 */
	for (; vaddr < PHYSMEM_END; vaddr += SIZE_1G) {
		boot_ttbr1_l1[GET_L1_INDEX(vaddr + KERNEL_VADDR)] =
			(vaddr) | UXN | ACCESSED | NG |
			DEVICE_MEMORY /* 设备内存 */
			| IS_VALID;
	}

	/* 二、配置 boot_ttbr0，映射低位虚拟内存，确保开启MMU后能正常访问，
         *     只需要映射boot init所在的区域，不会超过2MB, 还需映射外设内存，确保串口正常工作
         */

	vaddr = PHYSMEM_START; // 重新初始化 vaddr 为物理内存的起始地址
	boot_ttbr0_l0[GET_L0_INDEX(vaddr)] = ((u64)boot_ttbr0_l1) | IS_TABLE |
					     IS_VALID | NG;
	boot_ttbr0_l1[GET_L1_INDEX(vaddr)] = ((u64)boot_ttbr0_l2) | IS_TABLE |
					     IS_VALID | NG;
	for (; vaddr < SIZE_2M; vaddr += SIZE_2M) {
		boot_ttbr0_l2[GET_L2_INDEX(vaddr)] = (vaddr) | UXN | ACCESSED |
						     NG | INNER_SHARABLE |
						     NORMAL_MEMORY | IS_VALID;
	}
	for (; vaddr < SHARED_PERIPHERAL_END; vaddr += SIZE_2M) {
		boot_ttbr0_l2[GET_L2_INDEX(vaddr)] =
			(vaddr) | UXN | ACCESSED | NG | INNER_SHARABLE |
			DEVICE_MEMORY /* 设备内存 */
			| IS_VALID;
	}
}
