/*
 * @brief 设置ttbr0_el1寄存器，用来切换用户态进程的页表
 */
.global set_ttbr0_el1;
.type set_ttbr0_el1, %function;
set_ttbr0_el1:
	msr ttbr0_el1, x0
	/* 修改TTBRx_EL1或TCR_EL1后，需执行TLBI VMALLE1指令刷新TLB，但是如果启用ASID的话可以忽略 */
	// dsb sy	 // dsb数据同步指令，拥有内存屏障的功能且比dmb约束力更强
	//tlbi vmalle1is
	//dsb sy
	isb
	ret
.size set_ttbr0_el1, .- set_ttbr0_el1

