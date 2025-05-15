#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/kprint.h>
#include <arch/machine/smp.h>
#include <arch/mmu.h>

volatile char cpu_status[PLAT_CPU_NUM] = { cpu_hang };

struct per_cpu_info cpu_info[PLAT_CPU_NUM] __attribute__((aligned(64)));
u64 ctr_el0;

inline u32 smp_get_cpu_id(void)
{
	return get_per_cpu_info() - cpu_info;
}

inline struct per_cpu_info *get_per_cpu_info(void)
{
	struct per_cpu_info *info;

	asm volatile("mrs %0, tpidr_el1" : "=r"(info));

	return info;
}

/**
 * CRT_EL0: https://www.yuque.com/kingdo-8jdv5/xtd3c8/vzvd1xebpcpob8ns
 */
static inline u64 read_ctr(void)
{
	u64 reg;
	asm volatile("mrs %0, ctr_el0" : "=r"(reg)::"memory");
	return reg;
}

void init_per_cpu_info(u32 cpuid)
{
	struct per_cpu_info *info;

	if (cpuid == 0)
		ctr_el0 = read_ctr();

	info = &cpu_info[cpuid];

	// 设置当前 CPU 正在执行的上下文，0表示当前 CPU 没有执行任何线程
	info->cur_exec_ctx = 0;

	// 设置当前 CPU 的内核栈地址，内核栈的起始地址是 KSTACKx_ADDR(cpuid)，
	info->cpu_stack = (char *)(KSTACKx_ADDR(cpuid) + CPU_STACK_SIZE);

	info->fpu_owner = NULL;
	info->fpu_disable = 0;

	// 设置tpidr_el1寄存器，指向当前CPU的per_cpu_info结构体
	asm volatile("msr tpidr_el1, %0" ::"r"(info));
}

/**
 * https://www.yuque.com/kingdo-8jdv5/xtd3c8/ggpci8rg070r0gwu
 */
u64 smp_get_mpidr(void)
{
	u64 mpidr = 0;

	asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
	return mpidr;
}
