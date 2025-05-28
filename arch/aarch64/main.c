#include <arch/machine/smp.h>
#include <arch/machine/pmu.h>
#include <arch/mm/page_table.h>
#include <irq/irq.h>
#include <irq/timer.h>
#include <arch/boot.h>
#include <machine.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/kprint.h>
#include <common/lock.h>
#include <common/poweroff.h>
#include <mm/mm.h>

/* 临时内核栈，真正栈帧由KSTACKx_ADDR(cpuid)计算，此时还没有将其写入页表 */
char cpu_stacks[PLAT_CPU_NUM][CPU_STACK_SIZE] ALIGN(STACK_ALIGNMENT);
/* 用于存放ttbr0_el1的低位空白页表 */
char empty_page[4096] ALIGN(PAGE_SIZE);

struct lock big_kernel_lock;
void enable_irq(void);

/*
 * @boot_flag: 从核可以正式启动时设置的标志
 * @physmem_info: 物理内存信息
 */
void main(paddr_t boot_flag, void *physmem_info)
{
	u32 ret = 0;
	kinfo("Hello KingdoMicroKernel! I am in kernel now!\n");

	lock_init(&big_kernel_lock);
	kinfo("big_kernel_lock init finished\n");

	/* 初始化主核的信息 */
	init_per_cpu_info(0);
	kinfo("per-CPU info init finished\n");

	/* Init mm */
	mm_init(physmem_info);
	kinfo("mm init finished\n");

	/* 将内核栈映射到KSTACK_BASE以上的地址，确保发生栈溢出的时候不会破坏内核数据 */
	map_range_in_pgtbl_kernel(
		(void *)((unsigned long)boot_ttbr1_l0 + KBASE), KSTACKx_ADDR(0),
		(unsigned long)(cpu_stacks[0]) - KBASE, CPU_STACK_SIZE,
		VMR_READ | VMR_WRITE);

	/* 初始化 */
	arch_interrupt_init();
	timer_init();
	kinfo("interrupt init finished\n");

	pmu_init();
	kinfo("pmu init finished\n");

	/// 关机
	plat_poweroff();
}

void secondary_start(u32 cpuid)
{
	while (1)
		;
}