#include "consts.h"

.extern clear_bss_flag
.extern secondary_boot_flag
.extern arm64_elX_to_el1
.extern boot_cpu_stack
.extern init
.extern secondary_init

.section .text
.global _start;
.type _start, %function;
_start: 
	mrs     x8, mpidr_el1   // 加载系统寄存器mpidr_el1，其低8位表示CPU ID
	and	    x8, x8,	#0xFF   // 取出CPU ID 放入x8
	cbz	    x8, primary     // 如果CPU ID为0，跳转到primary，执行主核的代码

    /* 其他核心也进入 el1 特权级 */
    bl 	arm64_elX_to_el1

	/* 准备栈空间 */
	mov	x1, #INIT_STACK_SIZE
	mul	x1, x8, x1
	adr x0, boot_cpu_stack
	add	x0, x0, x1
	add	x0, x0, #INIT_STACK_SIZE
	mov	sp, x0

/* 等待SMP启用 */
wait_until_smp_enabled:
    mov	x1, #8                      
    mul	x2, x8, x1                  // x8 中存储的是 CPU ID, 乘以 8 表示每个 CPU 的 secondary_boot_flag 在数组中的字节偏移量
	ldr	x1, =secondary_boot_flag    // 加载 secondary_boot_flag 数组的基址
	add	x1, x1, x2                  // 计算每个 CPU 的 secondary_boot_flag 的地址
	ldr	x3, [x1]                    // 读取每个 CPU 的 secondary_boot_flag
	cbz	x3, wait_until_smp_enabled  // 如果 secondary_boot_flag 为 0，继续等待

    /* 调用 secondary_init 函数 */
	mov	x0, x8  // 将 CPU ID 传递给 secondary_init 函数
	bl 	secondary_init

	/* 永远不会到达这里 */
	b .

primary:
    /* 进入 el1 特权级 */
    bl 	arm64_elX_to_el1

    /* 准备栈空间，并跳转到C代码中 */
    adr     x0, boot_cpu_stack // 加载栈空间基址
    add     x0, x0, #INIT_STACK_SIZE // 栈顶地址
    mov     sp, x0 // 设置栈指针
    bl      init // 跳转到C代码中的main函数

    /* 永远不会到达这里 */
    b .
    
.size _start, .- _start
