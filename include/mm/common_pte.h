#ifndef MM_COMMON_PTE_H
#define MM_COMMON_PTE_H

#include <common/types.h>
#include <arch/mmu.h>

#define L0 (0)
#define L1 (1)
#define L2 (2)
#define L3 (3)

/**
 * @brief 架构无关的 PTE 结构，包含所有架构共享的一些有用信息。
 *        这个结构可以用来编写架构无关的代码来解析或操作 PTE。
 */
struct common_pte_t {
	unsigned long pfn; // 物理页号 page frame number
	vmr_prop_t perm;
	unsigned char valid : 1, // 有效位
		access : 1, // 访问位
		dirty : 1, // 脏位
		_unused : 4;
};

/**
 * @brief 将架构特定的 PTE 解析为通用 PTE。
 * @param pte [In] 指向架构特定 PTE 的指针
 * @param level [In] 页表中 @pte 所在的级别。
 * 
 * @warning: 当前，最后一级 PTE 编码为级别 3，级别 3 以上的级别编码为
 * 级别 2、1、0（如果架构有 4 级页表，否则最小级别为 1）。但是除 3 以外的级别
 * 现在不在这个函数中使用。如果我们将来要实现 5 级页表，我们应该更改
 * 级别的编码。
 */
void parse_pte_to_common(pte_t *pte, unsigned int level,
			 struct common_pte_t *ret);

/**
 * @brief 将通用 PTE 中的设置分配给架构特定的 PTE。
 * @param dest [In] 指向架构特定 PTE 的指针
 * @param level [In] 页表中 @dest 所在的级别。
 * @param src [In] 指向通用 PTE 的指针
*/
void update_pte(pte_t *dest, unsigned int level, struct common_pte_t *src);

#endif /* MM_COMMON_PTE_H */