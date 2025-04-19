#include <arch/mm/page_table.h>
#include <machine.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/kprint.h>

/* 临时内核栈，真正栈帧由KSTACKx_ADDR(cpuid)计算，此时还没有将其写入页表 */
char cpu_stacks[PLAT_CPU_NUM][CPU_STACK_SIZE] ALIGN(STACK_ALIGNMENT);
/* 用于存放ttbr0_el1的低位空白页表 */
char empty_page[4096] ALIGN(PAGE_SIZE);

/*
 * @boot_flag: 从核可以正式启动时设置的标志
 */
void main(paddr_t boot_flag)
{
    kinfo("Hello KingdoMicroKernel! I am in kernel now!\n");
    while (1);
}

void secondary_start(u32 cpuid)
{
    while (1);
}