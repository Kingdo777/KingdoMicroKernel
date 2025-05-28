
#ifndef ARCH_AARCH64_ARCH_MACHINE_PMU_H
#define ARCH_AARCH64_ARCH_MACHINE_PMU_H

#include <common/types.h>

/** 
 * PMCR_EL0 是 ARM64 架构中 ​​PMU（性能监控单元）的核心控制寄存器​​，用于全局配置 PMU 的行为。
 * 位域		名称		作用
 *  ​​0​​	     E	            启用所有计数器
​ *  ​1​​	     P	            复位事件计数器
 *  ​​2​​	     C	            复位 Cycle 计数器
​ *  ​3​​	     D	            时钟分频控制
​ *  ​4​​	     X	            启用事件流导出
​ *  ​5​​	     DP	            事件计数被禁用时禁用 Cycle 计数器
​ *  ​6​​	     LC	            设置64位长 Cycle 计数器
 * ​11-15​​	    N	           实现的事件计数器数量 (RO)
*/
#define PMCR_EL0_MASK (0x3f)
#define PMCR_EL0_E (1 << 0)
#define PMCR_EL0_P (1 << 1)
#define PMCR_EL0_C (1 << 2)
#define PMCR_EL0_D (1 << 3)
#define PMCR_EL0_X (1 << 4)
#define PMCR_EL0_DP (1 << 5)
#define PMCR_EL0_LC (1 << 6)
#define PMCR_EL0_N_SHIFT (11)
#define PMCR_EL0_N_MASK (0x1f)

/**
 * pmuserenr_el0（PMU 用户态使能寄存器），控制用户态（EL0）是否可以访问 PMU 寄存器​ ，下面
 * 的宏定义了 pmuserenr_el0 PMU相关的寄存器位，用于控制PMU的行为
 * - PMUSERENR_EL0_EN：​​允许用户态访问 PMU 寄存器​​（否则访问会触发 UNDEFINED 异常）
 * - PMUSERENR_EL0_SW：​​软件增量（SW Increment）写入陷阱控制​​（置1允许用户态写入 PMSWINC_EL0，置0写入会触发陷阱）
 * - PMUSERENR_EL0_CR：​​Cycle 计数器（PMCCNTR_EL0）读取陷阱控制​​（置1允许用户态读取 PMCCNTR_EL0，置0读取会触发陷阱)
 * - PMUSERENR_EL0_ER：事件计数器（PMEVCNTRn_EL0）读取陷阱控制​​（置1允许用户态读取事件计数器，置0读取会触发陷阱）
 */
#define PMUSERENR_EL0_EN (1 << 0)
#define PMUSERENR_EL0_SW (1 << 1)
#define PMUSERENR_EL0_CR (1 << 2)
#define PMUSERENR_EL0_ER (1 << 3)

/**
 * PMCNTENSET_EL0 是 ARM64 架构中 ​​PMU（性能监控单元）的计数器使能寄存器​​，用于启用或禁用特定的性能计数器。
 * 位域		名称		作用
 *  ​​31​​	     C	       启用 Cycle 计数器（PMCCNTR_EL0），开始统计 CPU 周期数
​ * 0-30​	   -	     启用事件计数器（PMEVCNTRn_EL0），每个位对应一个事件计数器（例如，位 0 控制 PMEVCNTR0_EL0）
 */
#define PMCNTENSET_EL0_C (1 << 31)

void enable_cpu_cnt(void);
void disable_cpu_cnt(void);
void pmu_init(void);

/**
 * @brief 读取 PMCCNTR_EL0 寄存器的值，返回当前 CPU 周期计数
 * @return 当前 CPU 周期计数
 */
static inline u64 pmu_read_real_cycle(void)
{
	s64 tv;
	asm volatile("mrs %0, pmccntr_el0" : "=r"(tv));
	return tv;
}

/**
 * @brief 清除 PMCCNTR_EL0 寄存器的计数值，将其重置为 0
 * @note 这将清除当前的 CPU 周期计数
 */
static inline void pmu_clear_cnt(void)
{
	asm volatile("msr pmccntr_el0, %0" ::"r"(0));
}

#endif /* ARCH_AARCH64_ARCH_MACHINE_PMU_H */