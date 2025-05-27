#include <irq/irq.h>
#include <common/kprint.h>
#include <io/uart.h>
#include <machine.h>
#include <common/types.h>
#include <common/macro.h>
#include <arch/machine/smp.h>
#include <arch/sync.h>
#include <arch/tools.h>

/*
 * Refer to BROADCOM BCM2835 manual (no official BCM2837 manual).
 * The IRQ details are the same on 2835 and 2837.
 */
#define IRQ_USB (9)
#define IRQ_USB_BIT (1 << IRQ_USB)
#define IRQ_SDIO (32 + 24)
#define IRQ_SDIO_BIT (1 << (IRQ_SDIO - 32))
#define IRQ_UART (32 + 25)
#define IRQ_UART_BIT (1 << (IRQ_UART - 32))
#define IRQ_ARASANSDIO (32 + 30)
#define IRQ_ARASANSDIO_BIT (1 << (IRQ_ARASANSDIO - 32))

/* Per core IRQ SOURCE MMIO address */
u64 core_irq_source[PLAT_CPU_NUM] = {
	CORE0_IRQ_SOURCE,
	CORE1_IRQ_SOURCE,
	CORE2_IRQ_SOURCE,
	CORE3_IRQ_SOURCE,
};

/**
 * @brief 初始化树莓派（基于 ​​BCM2835​​ 芯片）的​​中断控制器​
 * BCM2835 的中断源被分为三类，对应不同的寄存器组：
 * - ​​IRQ1​​：外设中断（Peripheral IRQs），如USB、DMA、GPIO等
 *   对应寄存器：ENABLE1/DISABLE1/PENDING1
​​ * - IRQ2​​：GPU相关中断（GPU IRQs），如GPU生成的信号
 *   对应寄存器：ENABLE2/DISABLE2/PENDING2
​ * - ​BASIC​​：基础中断（Basic IRQs），如系统定时器、邮箱中断等。
 *   对应寄存器：ENABLE_BASIC/DISABLE_BASIC/PENDING_BASIC。
 */
static void interrupt_init(void)
{
	static int once = 1; // 确保初始化只执行一次

	if (once == 1) {
		once = 0;

		/* 在初始化阶段关闭所有中断，避免未配置的中断意外触发 */
		put32(BCM2835_IRQ_FIQ_CTRL, 0); // 禁用FIQ和IRQ
		put32(BCM2835_IRQ_DISABLE1,
		      (u32)-1); // 禁用所有IRQ1类型中断，如USB、DMA等
		put32(BCM2835_IRQ_DISABLE2,
		      (u32)-1); // 禁用所有IRQ2类型中断，如GPU相关中断
		put32(BCM2835_IRQ_DISABLE_BASIC,
		      (u32)-1); // 禁用所有BASIC类型中断，如系统定时器、邮箱中断等

		/* 通过“读-写回”操作清除未处理的中断（ACK）,避免误触发 */
		put32(BCM2835_IRQ_BASIC, get32(BCM2835_IRQ_BASIC));
		put32(BCM2835_IRQ_PENDING1, get32(BCM2835_IRQ_PENDING1));
		put32(BCM2835_IRQ_PENDING2, get32(BCM2835_IRQ_PENDING2));

		/* 启用USB中断 */
		plat_enable_irqno(IRQ_USB);

#if USE_mini_uart == 0
		/* Enable uart (pl011) irq */
		enable_uart_irq(IRQ_UART);
#endif

		isb();
		smp_mb();
	}
}

void plat_interrupt_init(void)
{
	interrupt_init(); // 初始化中断
}

void plat_enable_irqno(int irq)
{
	if (irq < 32)
		put32(BCM2835_IRQ_ENABLE1, (1 << irq));
	else if (irq < 64)
		put32(BCM2835_IRQ_ENABLE2, (1 << (irq - 32)));
}

void plat_disable_irqno(int irq)
{
	if (irq < 32)
		put32(BCM2835_IRQ_DISABLE1, (1 << irq));
	else if (irq < 64)
		put32(BCM2835_IRQ_DISABLE2, (1 << (irq - 32)));
}

void plat_handle_irq(void)
{
	u32 cpuid = 0;
	unsigned int irq;

	cpuid = smp_get_cpu_id();

	/* By default, interrupts are routed to CPU 0. */
	BUG_ON(cpuid != 0);

	irq = get32(BCM2835_IRQ_PENDING2);
	if (irq != 0) {
#if USE_mini_uart == 0
		if (irq & IRQ_UART_BIT) {
			uart_irq_handler();
		}
#endif /* By default, PL011 is used */
	}

	return;
}