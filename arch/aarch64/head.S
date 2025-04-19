.extern cpu_stacks;
.extern flush_tlb_all;
.extern main;
.extern secondary_start;
.extern empty_page;

.text
/**
 * @brief 从Boot init程序跳转到内核的入口函数
 * @param secondary_boot_flag 从核的启动标志
 */
.global start_kernel;
.type start_kernel, %function;
start_kernel:
    ldr     x2, =cpu_stacks         // 获取主核的基址
    add	    x2, x2, #0x1, lsl #12   // 计算栈顶地址
    mov	    sp, x2                  // 设置栈指针
    stp	    x0, x1, [sp, #-16]!     // 保存x0和x1寄存器的值到栈中
    adrp    x2, empty_page
    msr	    ttbr0_el1, x2           // 此时以及进入内核高地址，原先为执行init设置的低位页表没有用了
    isb                             // 同步指令
    bl      flush_tlb_all           // 清除数据缓存
    ldp     x0, x1, [sp], #16       // 恢复x0和x1寄存器的值，作为参数传递给内核的入口函数
    bl      main                    // 跳转到内核的入口函数
    // 永远不会到达这里
    b       .
.size start_kernel, .- start_kernel

/**
 * @brief 从核的启动函数
 * @param cpuid 从核的ID
 */
.global secondary_cpu_boot;
.type secondary_cpu_boot, %function;
secondary_cpu_boot:
    mov	    x19, x0                 // 保存从核的ID
    mov	    x1, #0x1000             // 4096
    mul	    x2, x0, x1              // 计算从核的栈空间偏移量
    ldr	    x3, =cpu_stacks          // 获取从核的栈空间基址
    add	    x2, x2, x3              // 计算从核的栈空间地址
    add	    x2, x2, #0x1, lsl #12   // 计算栈顶地址
    mov	    sp, x2                  // 设置栈指针
    adrp    x3, empty_page
    msr	    ttbr0_el1, x3           // 清空从核低位页表
    isb
    bl	    flush_tlb_all           // 清除数据缓存
    mov	    x0, x19
    bl	    secondary_start         // 跳转到从核的入口函数
    // 永远不会到达这里
    b       .
.size secondary_cpu_boot, .- secondary_cpu_boot

