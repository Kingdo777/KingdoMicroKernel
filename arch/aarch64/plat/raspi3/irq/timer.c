#include <irq/irq.h>
#include <irq/timer.h>
#include <arch/machine/smp.h>
#include <common/kprint.h>
#include <common/list.h>
#include <common/lock.h>
#include <arch/tools.h>

u64 cntp_init; // 初始计数器值
u64 cntp_freq; // 计数器频率(Hz), 即每秒钟tick数
u64 cntp_tval; // 定时器计数值，即间隔多久产生一次中断
u64 tick_per_us; // 每微秒的tick数

/* 每个CPU核心的定时器中断控制寄存器地址 */
u64 core_timer_irqcntl[PLAT_CPU_NUM] = { CORE0_TIMER_IRQCNTL, CORE1_TIMER_IRQCNTL, CORE2_TIMER_IRQCNTL,
					 CORE3_TIMER_IRQCNTL };

void plat_timer_init(void)
{
	u64 count_down = 0;
	u64 timer_ctl = 0;
	u32 cpuid = smp_get_cpu_id();

	/**
	 * 读取ARM系统计数器
	 * - 读取cntpct_el0寄存器（物理计数器值）存储到cntp_init
	 * - 读取cntfrq_el0寄存器（计数器频率）存储到cntp_freq
	 */
	asm volatile("mrs %0, cntpct_el0" : "=r"(cntp_init));
	kdebug("timer init cntpct_el0 = %lu\n", cntp_init);
	asm volatile("mrs %0, cntfrq_el0" : "=r"(cntp_freq));
	kdebug("timer init cntfrq_el0 = %lu\n", cntp_freq);

	/**
	 * 设置定时器初始值
	 * - TICK_MS表示每次定时器中断的间隔时间（毫秒）
	 * - cntp_freq / 1000即每毫秒的tick数
	 * - cntp_tval即每次定时器中断需要的tick数
	 */
	cntp_tval = (cntp_freq * TICK_MS / 1000);
	kinfo("CPU freq %lu, set timer %lu\n", cntp_freq, cntp_tval);

	tick_per_us = cntp_freq / 1000 / 1000;

	/* 设置定时器值，到期后产生一次中断 */
	asm volatile("msr cntp_tval_el0, %0" ::"r"(cntp_tval));
	asm volatile("mrs %0, cntp_tval_el0" : "=r"(count_down));
	kdebug("timer init cntp_tval_el0 = %lu\n", count_down);

	/* 配置定时器中断（CNTPNSIRQ，即非安全异常级el1/el0的物理定时器） */
	put32(core_timer_irqcntl[cpuid], INT_SRC_TIMER1);

	/**
	 * 启用定时器
	 * - 位0(ENABLE)：1（启用定时器）
	 * - 位1(IMASK)：0（不屏蔽中断）
	 */
	timer_ctl = 0 << 1 | 1; /* IMASK = 0 ENABLE = 1 */
	asm volatile("msr cntp_ctl_el0, %0" ::"r"(timer_ctl));
	asm volatile("mrs %0, cntp_ctl_el0" : "=r"(timer_ctl));
	kdebug("timer init cntp_ctl_el0 = %lu\n", timer_ctl);
	return;
}

void plat_set_next_timer(u64 tick_delta)
{
	asm volatile("msr cntp_tval_el0, %0" ::"r"(tick_delta));
}

void plat_set_init_timer(void)
{
	asm volatile("msr cntp_tval_el0, %0" ::"r"(cntp_tval));
}

/**
 * @brief 获取单调时间, 即从系统启动到现在的时间，单位为纳秒
 */
u64 plat_get_mono_time(void)
{
	u64 cur_cnt = 0;
	asm volatile("mrs %0, cntpct_el0" : "=r"(cur_cnt));
	return (cur_cnt - cntp_init) * NS_IN_US / tick_per_us;
}
/**
 * @brief 获取当前tick值
 */
u64 plat_get_current_tick(void)
{
	u64 cur_cnt = 0;
	asm volatile("mrs %0, cntpct_el0" : "=r"(cur_cnt));
	return cur_cnt;
}
