#include <arch/machine/registers.h>

/* 定义不同特权级的值, https://developer.arm.com/documentation/ddi0601/2020-12/AArch64-Registers/CurrentEL--Current-Exception-Level */
#define CURRENTEL_EL0           (0b00 << 2)
#define CURRENTEL_EL1           (0b01 << 2)
#define CURRENTEL_EL2           (0b10 << 2)
#define CURRENTEL_EL3           (0b11 << 2)

#define CPACR_EL1_FPEN          (0b11 << 20)
#define ID_AA64PFR0_EL1_GIC     (0b1111 << 24)

#define CNTHCTL_EL2_EL1PCEN     (1 << 1)
#define CNTHCTL_EL2_EL1PCTEN    (1 << 0)
#define CPTR_EL2_RES1           0x33ff
#define HCR_EL2_RW              (1 << 31)
#define ICC_SRE_EL2_SRE         (1 << 0)
#define ICC_SRE_EL2_ENABLE      (1 << 3)

/* SCR_EL3 寄存器 的位域定义 
 * NS: Non-secure state, 表示是否处于非安全状态, 1 表示非安全状态
 * HCE: Hypervisor Call Enable, 表示是否启用 HVC 指令, 1 表示启用, 即允许EL0/EL1/EL2 调用 hypervisor call (HVC 指令) 时陷入 EL2
 * RW: Register Width, 表示寄存器宽度, 1 表示 64 位寄存器, 强制 ​​EL2 和 EL1​​ 运行在 ​​AArch64 模式​​（64 位指令集）
 */
#define SCR_EL3_NS              (1 << 0)
#define SCR_EL3_HCE             (1 << 8)
#define SCR_EL3_RW              (1 << 10)

/* SPSR_ELX 寄存器 的位域定义 
 * M[3:0]: ​​目标异常级别和栈指针模式​​, 0101 表示返回到 EL1, 使用内核态栈指针SP_EL1
 * DAIF(Bits [9:6]): ​​中断屏蔽位​​（控制 Debug、SError、IRQ、FIQ 是否屏蔽）
 */
#define SPSR_ELX_DAIF           (0b1111 << 6)
#define SPSR_ELX_EL1H           (0b0101)

#define ICH_HCR_EL2             S3_4_C12_C11_0
#define ICC_SRE_EL2             S3_4_C12_C9_5

.section .text

.global arm64_elX_to_el1;
.type arm64_elX_to_el1, %function;
arm64_elX_to_el1:
	/* 获取当前CPU的特权级 */
	mrs x9, CurrentEL	// 系统寄存器 CurrentEL 存放当前特权级
	and x9, x9, #0b1100	// bits [3:2] 表示当前特权级, 

	/* 根据当前特权级进行不同的处理 */
	cmp x9, CURRENTEL_EL1
	beq .Ltarget			// 如果当前特权级为 EL1，跳转到 .Ltarget
	cmp x9, CURRENTEL_EL2
	beq .Lin_el2			// 如果当前特权级为 EL2，跳转到 .Lin_el2
	// 其他情况下, 说明在 EL3

	/* 设置 64 位寄存器宽度, 并启用 HVC 指令 */
	mrs x9, scr_el3	// 读取 SCR_EL3 寄存器, 负责控制 ​​EL3（Secure Monitor）​​ 的行为
	mov x10, SCR_EL3_NS | SCR_EL3_HCE | SCR_EL3_RW
	orr x9, x9, x10
	msr scr_el3, x9 // 写入 SCR_EL3 寄存器

	/*
	 * elr_el3 寄存器负责记录当 CPU 从低异常级别（如 EL1/EL2）通过异常进入 EL3 时发生异常时的下一条指令地址​​（即返回地址）,
	 * 因此可以通过设置 elr_el3 寄存器来控制 CPU 从 EL3 返回到 EL1/EL2 时的下一条指令地址
	 *
	 * spsr_el3 寄存器负责保存异常发生时的状态信息, 包括当前的异常级别、异常类型、异常标志等,
	 * 因此可以通过设置 spsr_el3 寄存器来控制 CPU 从 EL3 返回到 EL1/EL2 时的状态信息
	 * 这里设置了 SPSR_ELX_DAIF 和 SPSR_ELX_EL1H, 其中 SPSR_ELX_DAIF 表示禁用中断和异常, SPSR_ELX_EL1H 表示返回到 EL1
	 */
	adr x9, .Ltarget
	msr elr_el3, x9
	mov x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
	msr spsr_el3, x9

.Lin_el2:
	// 允许 EL1 直接管理物理计时器，并同步时间偏移
	mrs x9, cnthctl_el2
	orr x9, x9, CNTHCTL_EL2_EL1PCEN | CNTHCTL_EL2_EL1PCTEN
	msr cnthctl_el2, x9
	msr cntvoff_el2, xzr

	// 关闭二阶段地址翻译，也就是关闭内存虚拟化，直接使用 EL1 页表
	msr vttbr_el2, xzr

	// Disable EL2 coprocessor traps.
	mov x9, CPTR_EL2_RES1
	msr cptr_el2, x9

	// Disable EL1 FPU traps.
	mov x9, CPACR_EL1_FPEN
	msr cpacr_el1, x9

	// Check whether the GIC system registers are supported.
	mrs x9, id_aa64pfr0_el1
	and x9, x9, ID_AA64PFR0_EL1_GIC
	cbz x9, .Lno_gic_sr

	// Enable the GIC system registers in EL2, and allow their use in EL1.
	mrs x9, ICC_SRE_EL2
	mov x10, ICC_SRE_EL2_ENABLE | ICC_SRE_EL2_SRE
	orr x9, x9, x10
	msr ICC_SRE_EL2, x9

	// Disable the GIC virtual CPU interface.
	msr ICH_HCR_EL2, xzr

.Lno_gic_sr:
	// Set EL1 to 64bit.
	mov x9, HCR_EL2_RW
	msr hcr_el2, x9

	/* 类似于 SCR_EL3 和 SPSR_EL3 寄存器 */
	adr x9, .Ltarget
	msr elr_el2, x9
	mov x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
	msr spsr_el2, x9

	isb		// ​​指令同步屏障, 确保所有之前的指令都已经完成
	eret	// 返回到 EL1, 并且使用 elr_el3 寄存器中保存的地址作为下一条指令地址, 即 .Ltarget

.Ltarget:
	ret
.size arm64_elX_to_el1, .- arm64_elX_to_el1

// See https://developer.arm.com/documentation/den0024/a/Caches/Cache-maintenance
.global invalidate_cache_all;
.type invalidate_cache_all, %function;
invalidate_cache_all:
	mrs     x0, clidr_el1
	and     w3, w0, #0x07000000     // get 2x level of coherence
	lsr     w3, w3, #23
	cbz     w3, .Lfinished_inv_cache
	mov     w10, #0                 // w10 = 2x cache level
	mov     w8, #1                  // w8 = constant 1
.Lloop1_inv_cache:
	add     w2, w10, w10, lsr #1    // calculate 3x cache level
	lsr     w1, w0, w2              // extract 3 bit cache type for this level
	and     w1, w1, #0x7
	cmp     w1, #2
	b.lt    .Lskip_inv_cache        // no data or unified cache at this level
	msr     csselr_el1, x10         // select this cache level
	isb                             // synchronize change to csselr
	mrs     x1, ccsidr_el1          // w1 = ccsidr
	and     w2, w1, #7              // w2 = log2(line len) - 4
	add     w2, w2, #4              // w2 = log2(line len)
	ubfx    w4, w1, #3, #10         // w4 = max way number, right aligned
	clz     w5, w4                  // w5 = 32 - log2(ways), bit position of way in DC operand
	lsl     w9, w4, w5              // w9 = max way number, aligned to position in DC operand
	lsl     w12, w8, w5             // w12 = amount to decrement way number per iteration

.Lloop2_inv_cache:
	ubfx    w7, w1, #13, #15        // w7 = max set number, right aligned
	lsl     w7, w7, w2              // w7 = max set number, aligned to position in DC operand
	lsl     w13, w8, w2             // w13 = amount to decrement set number per iteration
.Lloop3_inv_cache:
	orr     w11, w10, w9            // w11 = combine way number and cache number
	orr     w11, w11, w7            //       and set number for DC operand
	dc      isw, x11                // data cache op
	subs    w7, w7, w13             // decrement set number
	b.ge    .Lloop3_inv_cache

	subs    x9, x9, x12             // decrement way number
	b.ge    .Lloop2_inv_cache
.Lskip_inv_cache:
	add     w10, w10, #2            // increment 2x cache level
	cmp     w3, w10
	dsb     sy                      // ensure completetion of previous cache maintainance instructions
	b.gt    .Lloop1_inv_cache
.Lfinished_inv_cache:

	// dump the instruction cache as well
	ic      iallu
	isb
	ret
.size invalidate_cache_all, .- invalidate_cache_all

.extern boot_ttbr0_l0
.extern boot_ttbr1_l0

/* DEVICE_nGnRnE */
#define MMU_MAIR_ATTR0		(0x00 << (8 * 0))

/* DEVICE_nGnRE */
#define MMU_MAIR_ATTR1		(0x04 << (8 * 1))

/* DEVICE_GRE */
#define MMU_MAIR_ATTR2		(0x0c << (8 * 2))

/* NORMAL_NC */
#define MMU_MAIR_ATTR3          (0x44 << (8 * 3))

/* NORMAL */
#define MMU_MAIR_ATTR4          (0xff << (8 * 4))

/*
 * Enable cached page table walks:
 * inner/outer (IRGN/ORGN): write-back + write-allocate
 */
#define MMU_TCR_TG1_4k 			(0 << 14)
#define MMU_TCR_SH1_INNER_SH	(3 << 28)
#define MMU_TCR_ORGN1_WBA		(1 << 26)
#define MMU_TCR_IRGN1_WBA		(1 << 24)
#define MMU_TCR_T1SZ			((64 - 48) << 16) /* 48-bit  */
#define MMU_TCR_FLAGS1			(MMU_TCR_TG1_4k | MMU_TCR_SH1_INNER_SH | \
 						MMU_TCR_ORGN1_WBA | MMU_TCR_IRGN1_WBA | MMU_TCR_T1SZ)

#define MMU_TCR_TG0_4k 			(0 << 30)
#define MMU_TCR_SH0_INNER_SH	(3 << 12)
#define MMU_TCR_ORGN0_WBA		(1 << 10)
#define MMU_TCR_IRGN0_WBA		(1 << 8)
#define MMU_TCR_T0SZ			((64 - 48) << 0) /* 48-bit */
#define MMU_TCR_FLAGS0			(MMU_TCR_TG0_4k | MMU_TCR_SH0_INNER_SH | \
 						MMU_TCR_ORGN0_WBA | MMU_TCR_IRGN0_WBA | MMU_TCR_T0SZ)
#define MMU_TCR_IPS 			(0b101 << 32) /* 48-bit */
#define MMU_TCR_AS				(1 << 36)

.global el1_mmu_activate;
.type el1_mmu_activate, %function;
el1_mmu_activate:
	stp     x29, x30, [sp, #-16]!
	mov     x29, sp

	/* 让所有缓存失效 */
	bl	invalidate_cache_all

	/* 让TLB失效 */
	tlbi    vmalle1is
	isb
	dsb     sy

	/* Initialize Memory Attribute Indirection Register */
	ldr 	x8, =MMU_MAIR_ATTR0 | MMU_MAIR_ATTR1 | MMU_MAIR_ATTR2 | MMU_MAIR_ATTR3 | MMU_MAIR_ATTR4
	msr     mair_el1, x8

	/* 初始化 TCR_EL1 */
	/* set cacheable attributes on translation walk */
	/* (SMP extensions) non-shareable, inner write-back write-allocate */
	ldr  	x8, =MMU_TCR_FLAGS1 | MMU_TCR_FLAGS0 | MMU_TCR_IPS | MMU_TCR_AS
	msr     tcr_el1, x8
	isb

	/* 将 L0 页表页基址分别写入 ttbr0/ttbr1 寄存器*/
	adrp    x8, boot_ttbr0_l0
	msr     ttbr0_el1, x8
	adrp    x8, boot_ttbr1_l0
	msr     ttbr1_el1, x8
	isb

	/* 使能 MMU */
	mrs     x8, sctlr_el1
	orr		x8, x8, #SCTLR_EL1_M	// 开启 MMU
	bic     x8, x8, #SCTLR_EL1_A	// 禁用地址对齐检查，允许非对齐内存访问，软件层面​​禁用对齐异常（不报错）
	bic     x8, x8, #SCTLR_EL1_SA0	// 允许 EL0 使用未对齐的栈指针
	bic     x8, x8, #SCTLR_EL1_SA	// 允许 EL1 使用未对齐的栈指针
	orr     x8, x8, #SCTLR_EL1_nAA	// 强制对齐访问​，​​硬件层面​​优化非对齐访问（可能拆分为多次对齐操作）
	orr     x8, x8, #SCTLR_EL1_C	// 开启数据缓存
	orr     x8, x8, #SCTLR_EL1_I	// 开启指令缓存
	msr     sctlr_el1, x8

	ldp     x29, x30, [sp], #16
	ret
.size el1_mmu_activate, .- el1_mmu_activate


.global early_put32;
.type early_put32, %function;
early_put32:
	str w1, [x0]
	ret
.size early_put32, .- early_put32

.global early_get32;
.type early_get32, %function;
early_get32:
	ldr w0, [x0]
	ret
.size early_get32, .- early_get32

.global delay;
.type delay, %function;
delay:
	subs x0, x0, #1
	bne delay
	ret
.size delay, .- delay
