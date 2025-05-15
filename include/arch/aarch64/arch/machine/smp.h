#ifndef ARCH_AARCH64_ARCH_MACHINE_SMP_H
#define ARCH_AARCH64_ARCH_MACHINE_SMP_H

#ifndef __ASM__
#include <common/vars.h>
#include <machine.h>
#include <common/types.h>
#include <common/macro.h>

enum cpu_state {
	cpu_hang = 0, // CPU处于挂起状态, 表示CPU因严重错误而停止响应
	cpu_run = 1, // CPU处于运行状态, 表示CPU正常执行指令
	cpu_idle = 2 // CPU处于低功耗待机模式，没有任务需要处理
};
#endif

/**
 * ​​per-CPU 结构体（struct per_cpu_info）​​ 的偏移量宏。
 * 这些偏移量是硬编码的，意味着如果 struct per_cpu_info 的
 * 布局发生变化，这些宏也必须相应更新。
 */

#define OFFSET_CURRENT_EXEC_CTX 0
#define OFFSET_LOCAL_CPU_STACK 8
#define OFFSET_CURRENT_FPU_OWNER 16
#define OFFSET_FPU_DISABLE 24

#ifndef __ASM__

/**
 * 这是一个 ​​每个 CPU 核心独立维护的数据结构​​，存储与该 CPU 相关
 * 的状态信息（如当前执行的上下文、栈指针、FPU 所有者等）。
 * 其​​基地址存储在 TPIDR_EL1 寄存器​​（ARM 架构的线程指针寄存器）中，供快速访问。
 */
struct per_cpu_info {
	/// 指向当前 CPU 正在执行的上下文（线程或进程）的指针
	u64 cur_exec_ctx;
	/// 当前 CPU 的 ​内核栈地址​​，用于中断或异常处理时切换栈
	char *cpu_stack;
	/* struct thread *fpu_owner */
	void *fpu_owner;
	u32 fpu_disable;
	/// 填充到缓存行大小的字节，以确保结构体对齐
	char pad[pad_to_cache_line(sizeof(u64) + sizeof(char *) +
				   sizeof(void *) + sizeof(u32))];
} __attribute__((packed, aligned(64))); // 紧凑存储、对齐到 64 字节

void init_per_cpu_info(u32 cpuid);

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];
extern volatile char cpu_status[PLAT_CPU_NUM];
extern u64 ctr_el0;

void init_per_cpu_info(u32 cpuid);
struct per_cpu_info *get_per_cpu_info(void);
u32 smp_get_cpu_id(void);
u64 smp_get_mpidr(void);

#endif /* __ASM__ */

#endif /* ARCH_AARCH64_ARCH_MACHINE_SMP_H */