.text

.global flush_dcache_area;
.type flush_dcache_area, %function;
flush_dcache_area:
    mrs     x3, ctr_el0     // 读取 Cache Type Register (CTR_EL0)，获取缓存信息
    ubfx	x3, x3, #16, #4 // 从x3的第16位开始提取4位，获取​对数形式的缓存行大小​​（Log4 of cache line size）
    mov	    x2, #0x4
    lsl	    x2, x2, x3      // 计算缓存行大小，x2 = 0x4 << x3
    add	    x1, x0, x1      // 计算结束地址，x1 = 起始地址(x0) + 大小(x1)
    sub	    x3, x2, #0x1    // 计算掩码，x3 = 缓存行大小 - 1
    bic	    x0, x0, x3      // 对齐起始地址，x0 = 起始地址 & ~掩码
flsuh:    
    dc	    civac, x0       // 执行缓存清理指令，将 x0 地址对应的缓存行清理并无效化（Clean and Invalidate）
    add	    x0, x0, x2      // 计算下一个缓存行地址，x0 = x0 + 缓存行大小
    cmp	    x0, x1          // 比较当前地址和结束地址
    b.cc	flsuh           // 如果当前地址小于结束地址，则继续循环
    dsb     sy              // 确保所有缓存操作完成
    isb                     // 同步指令
    ret
.size flush_dcache_area, .- flush_dcache_area

.global flush_tlb_all;
.type flush_tlb_all, %function;
flush_tlb_all:
    dsb     sy              // 确保所有内存操作完成
    tlbi    vmalle1         // 清除所有进程的TLB条目
    dsb     sy              // 确保所有TLB操作完成
    isb                     // 同步指令
    ret
.size flush_tlb_all, .- flush_tlb_all

.global put32;
.type put32, %function;
put32:
	str w1, [x0]
	ret
.size put32, .- put32

.global get32;
.type get32, %function;
get32:
	ldr w0, [x0]
	ret
.size get32, .- get32
