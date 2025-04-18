#include "consts.h"

.extern boot_cpu_stack
.extern init

.section .text
.global _start;
.type _start, %function;
_start: 
	mrs     x8, mpidr_el1   // 加载系统寄存器mpidr_el1，其低8位表示CPU ID
	and	    x8, x8,	#0xFF   // 取出CPU ID 放入x8
	cbz	    x8, primary     // 如果CPU ID为0，跳转到primary，执行主核的代码
	b	    .				// 如果CPU ID不为0，跳转到死循环

primary:
	/* 准备栈空间，并跳转到C代码中 */
	adr     x0, boot_cpu_stack 		 // 加载栈空间基址
    add     x0, x0, #INIT_STACK_SIZE // 栈顶地址
    mov     sp, x0 					 // 设置栈指针
    bl      init 					 // 跳转到C代码中的init函数

.size _start, .- _start