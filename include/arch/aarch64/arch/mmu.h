/*
 *	aarch64 kernel virtual memory mapping layout (use raspi3 as example)
 *    +---DIRECT MAPPING_start---+  0xFFFFFF0000000000 (KBASE)
 *    |       kernel code        |
 *    |                          |
 *    +--------------------------+  img_end
 *    |         kmalloc          |
 *    |                          |
 *    +--------------------------+  0xFFFFFF003F000000 (KBASE + PERIPHERAL_BASE)
 *    |  peripheral mapping      |
 *    |                          |
 *    +----DIRECT MAPPING_end----+  0xFFFFFF0040000000 (KBASE + PERIPHERAL_END)
 *    |        ......            |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFF00000000 (KSTACK_BASE)
 *    |       kernel stack       |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFF00008000 (KSTACK_END)
 *    |        ......            |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFFFFFFFFFF
*/

#ifndef ARCH_AARCH64_ARCH_MMU_H
#define ARCH_AARCH64_ARCH_MMU_H

#include <common/vars.h>

#ifndef KBASE
#define KBASE 0xFFFFFF0000000000
#define PHYSICAL_ADDR_MASK (40)
#endif // 内核基址

#ifndef KSTACK_BASE
#define KSTACK_BASE 0xFFFFFFFF00000000
#define KSTACKx_ADDR(cpuid) ((cpuid) * 2 * CPU_STACK_SIZE + KSTACK_BASE)
#endif // 内核栈基址

#ifndef __ASM__

#include <arch/mm/page_table.h>

#define phys_to_virt(x) ((vaddr_t)((paddr_t)(x) + KBASE))
#define virt_to_phys(x) ((paddr_t)((vaddr_t)(x) - KBASE))

#endif // __ASM__

#endif /* ARCH_AARCH64_ARCH_MMU_H */