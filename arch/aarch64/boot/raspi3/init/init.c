#include "boot.h"
#include "image.h"
#include "consts.h"
#include <common/types.h>

char boot_cpu_stack[PLAT_CPU_NUMBER][INIT_STACK_SIZE] ALIGN(4096);

/* 从核开始执行的标志，被设置为{0xFFFF， 0， 0， 0， ...} 时 */
long secondary_boot_flag[PLAT_CPU_NUMBER] = {0xFFFF};

/*
 * 该函数用于手动清除 BSS 段的内容.
 * .bss段用于存储未初始化的全局变量和静态变量，将这部分值统一设置为0
 */
static void clear_bss(void)
{
	u64 bss_start_addr;
	u64 bss_end_addr;
	u64 i;

	bss_start_addr = (u64)&_bss_start;
	bss_end_addr = (u64)&_bss_end;

	for (i = bss_start_addr; i < bss_end_addr; ++i)
		*(char *)i = 0;
}
/* 唤醒 ARM64 多核处理器中的非主核（Secondary Cores）
 * QEMU 在模拟机器启动时会同时开启 4 个 CPU 核心，4 个核会同时开始执行 _start 函数，
 * 但是在真机上，只有主核会执行 _start 函数，其他核会在 WFE（Wait For Event）状态下等待主核的唤醒。 
 */
static void wakeup_other_cores(void)
{
	u64 *addr;

	/*
	 * 将非主核的启动地址写入固定内存位置（0xe0, 0xe8, 0xf0）,
	 * 这些是 ​​硬编码的内存地址​​，通常由 ​​固件（如 armstub8.bin）​​ 预先定义，用于存储从核的启动地址
	 */
	addr = (u64 *)0xe0;
	*addr = TEXT_OFFSET;
	addr = (u64 *)0xe8;
	*addr = TEXT_OFFSET;
	addr = (u64 *)0xf0;
	*addr = TEXT_OFFSET;

	/*
	 * 通过 SEV（Set Event）指令唤醒处于 WFE（Wait for Event）状态的从核。
	 */
	asm volatile("sev");
}

/* 串口 */
void early_uart_init(void);
void uart_send_string(char *);

void init(void)
{
	/* Clear the bss area for the kernel image */
	clear_bss();

	/* Initialize UART before enabling MMU. */
	early_uart_init();
	uart_send_string("boot: init\r\n");

	wakeup_other_cores();

	/* Initialize Kernel Page Table. */
	uart_send_string("[BOOT] Install kernel page table\r\n");
	init_kernel_pt();

	/* Enable MMU. */
	el1_mmu_activate();
	uart_send_string("[BOOT] Enable el1 MMU\r\n");

	/* Call Kernel Main. */
	uart_send_string("[BOOT] Jump to kernel main\r\n");
	// start_kernel(secondary_boot_flag);

	/* Never reach here */
	while (1);
}

void secondary_init(int cpuid)
{
	el1_mmu_activate();
	// secondary_cpu_boot(cpuid);
}