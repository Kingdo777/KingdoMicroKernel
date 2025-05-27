// #include <common/vars.h>
#include <arch/machine/registers.h>
#include <arch/machine/esr.h>
#include <arch/machine/smp.h>

#include "irq_entry.h"

.extern syscall_table
.extern handle_sync
.extern handle_irq

/**
 * 定义异常向量的入口
 *  - label: 异常处理函数的标签
 * 该宏会在异常处理函数的入口处插入一个对齐指令，每个异常
 * 向量的大小是0x80，即128字节，因此需要对齐到7位
 */
.macro	exception_entry	label
	.align	7
	b	\label
.endm

/**
 * 处理异常前的现场保存和异常处理后的现场恢复
 *  - exception_enter: 保存现场
 *  - exception_exit: 恢复现场
 * ARCH_EXEC_CONT_SIZE 记录了所有要保存寄存器的大小
 *  - sp_el0: el0的sp指针
 *  - elr_el1: 在异常发生时，记录存储异常返回地址 (https://www.yuque.com/kingdo-8jdv5/xtd3c8/ym49rihi74dmxhbd)
 *  - spsr_el1: 在异常发生时，保存处理器的状态信息 (https://www.yuque.com/kingdo-8jdv5/xtd3c8/mplif2drwhavb39a)
 *  - x0-x30: 通用寄存器
 */
.macro	exception_enter
	sub	sp, sp, #ARCH_EXEC_CONT_SIZE
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	mrs	x21, sp_el0
	mrs	x22, elr_el1
	mrs	x23, spsr_el1
	stp	x30, x21, [sp, #16 * 15]
	stp	x22, x23, [sp, #16 * 16]
.endm

.macro	exception_exit
	ldp	x22, x23, [sp, #16 * 16]
	ldp	x30, x21, [sp, #16 * 15] 
	msr	sp_el0, x21
	msr	elr_el1, x22
	msr	spsr_el1, x23
	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]

	add	sp, sp, #ARCH_EXEC_CONT_SIZE
	eret
.endm

.macro switch_to_cpu_stack
	mrs     x24, TPIDR_EL1
	add	x24, x24, #OFFSET_LOCAL_CPU_STACK
	ldr	x24, [x24]
	mov	sp, x24
.endm

.macro switch_to_thread_ctx
	mrs     x24, TPIDR_EL1
	add	x24, x24, #OFFSET_CURRENT_EXEC_CTX
	ldr	x24, [x24]
	mov	sp, x24
.endm

/*
 * Vector table offsets from vector table base address from ARMv8 Manual
 * 第一、第二块是当前异常级的异常向量表，即处于EL1时触发的异常；当SPSel=0时，即
 * 栈指针使用SP_EL0时，使用第一块的异常向量表；当SPSel=1时，即栈指针使用SP_EL1时，
 * 使用第二块的异常向量表。
 * 第三、第四块是处于EL0时触发的EL1的异常，最典型的就是系统调用，即SVC异常。
 * 其中，EL0处于64位模式时，使用第三块的异常向量表；EL0处于32位模式时，使用第四块的异常
 * 向量表。
 * 异常类型有：
 *  - Synchronous: 同步异常
 *  - IRQ/vIRQ: 普通中断异常（虚拟普通中断）
 *  - FIQ/vFIQ: 快速中断异常（虚拟快速中断）
 *  - SError/vSError: 
 *	Address		|	Exception Type		| 	Description
 * ============================================================================
 *	VBAR_Eln+0x000	|	 Synchronous		|	 SPSel=0
 * 		+0x080	|	  IRQ/vIRQ		|	Current EL
 *		+0x100	|	  FIQ/vFIQ		|   with Stack Pointer
 * 		+0x180	|	SError/vSError		|    shared with EL0
 * ============================================================================
 *	VBAR_Eln+0x200	|	 Synchronous		|	 SPSel=1
 * 		+0x280	|	  IRQ/vIRQ		|	Current EL
 *		+0x300	|	  FIQ/vFIQ		|   with dedicated
 * 		+0x380	|	SError/vSError		|    Stack Pointer
 * ============================================================================
 *	VBAR_Eln+0x400	|	 Synchronous		|
 * 		+0x480	|	  IRQ/vIRQ		|	Lower EL
 *		+0x500	|	  FIQ/vFIQ		|    using AArch64
 * 		+0x580	|	SError/vSError		|
 * ============================================================================
 *	VBAR_Eln+0x600	|	 Synchronous		|
 * 		+0x680	|	  IRQ/vIRQ		|     	Lower EL
 *		+0x700	|	  FIQ/vFIQ		|    using AArch32
 * 		+0x780	|	SError/vSError		|
 * ============================================================================
 */

/**
 * el1_vector 的异常向量表，地址是11位对齐的，其地址存储需要存储在 VBAR_EL1 中
 */
.align	11
.globl el1_vector;
el1_vector:
	exception_entry	sync_el1t		// Synchronous EL1t
	exception_entry	irq_el1t		// IRQ EL1t
	exception_entry	fiq_el1t		// FIQ EL1t
	exception_entry	error_el1t		// Error EL1t

	exception_entry	sync_el1h		// Synchronous EL1h
	exception_entry	irq_el1h		// IRQ EL1h
	exception_entry	fiq_el1h		// FIQ EL1h
	exception_entry	error_el1h		// Error EL1h

	exception_entry	sync_el0_64		// Synchronous 64-bit EL0
	exception_entry	irq_el0_64		// IRQ 64-bit EL0
	exception_entry	fiq_el0_64		// FIQ 64-bit EL0
	exception_entry	error_el0_64		// Error 64-bit EL0

	exception_entry	sync_el0_32		// Synchronous 32-bit EL0
	exception_entry	irq_el0_32		// IRQ 32-bit EL0
	exception_entry	fiq_el0_32		// FIQ 32-bit EL0
	exception_entry	error_el0_32		// Error 32-bit EL0

/**
 * 内核态下使用SP_EL1，因此不处理 t 后缀的异常
 */
sync_el1t:
irq_el1t:
fiq_el1t:
error_el1t:
	bl unexpected_handler

/**
 * 不处理 el0 是aarch32 的异常
 */
sync_el0_32:
irq_el0_32:
fiq_el0_32:
error_el0_32:
	bl unexpected_handler

/**
 * 不处理fiq和error异常
 */
fiq_el1h:
error_el1h:
error_el0_64:
fiq_el0_64:
	bl unexpected_handler

/**
 * 普通irq异常处理，只处理：
 * - 处于el1且sp为SP_EL1
 * - 处于el0, 且位于64位模式
 */
irq_el1h:
irq_el0_64:
	exception_enter		// 保存现场
	bl	handle_irq	// 处理普通中断的入口函数
	exception_exit		// 恢复现场并执行eret返回

/**
 * 处于内核态(el1)且sp为SP_EL1时发生的同步异常
 *  - @param x0: 异常类型
 *  - @param x1: 异常状态寄存器，记录了具体异常原因
 *  - @param x2: 异常返回地址
 *  - @return: 返回地址，默认是x2的值，可能被修改
 */
sync_el1h:
	exception_enter		// 保存现场
	mov	x0, #SYNC_EL1h	// 异常类型为内核态同步异常
	mrs	x1, esr_el1	// 异常状态寄存器，记录了具体异常原因
	mrs	x2, elr_el1	// 异常返回地址
	bl	handle_sync	// 处理同步异常的入口函数
	str     x0, [sp, #16 * 16] // 将返回值写入到ELR_EL1中，用于异常返回的地址
	exception_exit		// 恢复现场并执行eret返回

/**
 * 处于el0, 且位于64位模式触发同步异常，如系统调用
 * - 如果是系统调用，则调用syscall_table中的相关处理函数，注意需要将返回值写到ctx中，
 *   从而在恢复现场时，写回给用户态线程。
 * - 如果是其他同步异常，则调用handle_sync处理，和sync_el1h区别在于需要切换到cpu内核栈
 */
sync_el0_64:
	exception_enter		// 保存现场
	switch_to_cpu_stack	// 切换到CPU内核栈
	mrs	x25, esr_el1	// 异常状态寄存器，记录了具体异常原因
	lsr	x24, x25, #ESR_EL1_EC_SHIFT
	cmp	x24, #ESR_EL1_EC_SVC_64 // 检查异常原因是否为系统调用
	b.eq	el0_syscall // 如果是系统调用，则跳转到el0_syscall处理
	mov	x0, SYNC_EL0_64 
	mrs	x1, esr_el1
	mrs	x2, elr_el1
	bl	handle_sync	// 处理同步异常的入口函数
	switch_to_thread_ctx
	exception_exit
el0_syscall:	// 如果同步异常的原因是系统调用
	adr	x27, syscall_table	// 将系统调用表的基地址加载到 x27
	uxtw	x16, w8			// 将32位的系统调用号零扩展为64位
	ldr	x16, [x27, x16, lsl #3]	// 根据系统调用号，从系统调用表中加载对应的函数地址
	blr	x16	// 调用系统调用处理函数
	switch_to_thread_ctx	// 切换到用户线程栈
	str	x0, [sp]	// 将系统调用的返回值存储到栈中
	exception_exit		// 恢复现场并执行eret返回	


/**
 * void eret_to_thread(u64 sp) 
 * 恢复用户态线程的上下文
 *  - @param sp: 用户态线程的栈指针
 *  - 恢复现场后，执行eret指令返回到用户态线程
 */
.globl __eret_to_thread
.type __eret_to_thread, %function
__eret_to_thread:
	mov	sp, x0
	dmb ish /* smp_mb() */
	exception_exit
.size __eret_to_thread, . - __eret_to_thread