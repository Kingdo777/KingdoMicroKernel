#include <common/kprint.h>
#include <arch/machine/pmu.h>

/**
 * PMU（Performance Monitoring Unit，性能监控单元），能够监控
 * CPU 的性能事件（如指令执行数、缓存命中/失效等）。在这是仅仅使能
 * PMU 的计数器功能，即统计 CPU 的周期数
 */
void pmu_init(void)
{
	enable_cpu_cnt();
	kdebug("pmu init current_cycle = %lu\n", pmu_read_real_cycle());
}

void enable_cpu_cnt(void)
{
	/* 允许用户态访问相关寄存器，而无需陷入到内核 */
	asm volatile("msr pmuserenr_el0, %0" ::"r"(
		PMUSERENR_EL0_EN // 允许用户态访问 PMU 寄存器
		| PMUSERENR_EL0_SW // 允许用户态写入 PMSWINC_EL0
		| PMUSERENR_EL0_CR // 允许用户态读取 PMCCNTR_EL0
		| PMUSERENR_EL0_ER // 允许用户态读取 PMEVCNTRn_EL0
		));
	/* 启用所有计数器，设置64位长 Cycle 计数器 */
	asm volatile("msr pmcr_el0, %0" ::"r"(PMCR_EL0_LC | PMCR_EL0_E));
	/* 启用Cycle计数器，开始统计 CPU 周期数 */
	asm volatile("msr pmcntenset_el0, %0" ::"r"(PMCNTENSET_EL0_C));
}

void disable_cpu_cnt(void)
{
	/* 禁用所有计数器 */
	asm volatile("msr pmcr_el0, %0" ::"r"(~PMCR_EL0_E));
	/* 禁用Cycle计数器 */
	asm volatile("msr pmcntenset_el0, %0" ::"r"(0));
}