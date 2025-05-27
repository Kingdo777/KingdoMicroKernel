#include <common/types.h>
#include <io/uart.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <common/kprint.h>
#include <common/lock.h>
#include <common/errno.h>
#include <common/poweroff.h>
#include <uapi/syscall_num.h>

/* Placeholder for system calls that are not implemented */
int sys_null_placeholder(void)
{
	kwarn("Invoke non-implemented syscall\n");
	return -EBADSYSCALL;
}

void sys_poweroff(void)
{
	plat_poweroff();
}

const void *syscall_table[NR_SYSCALL] = {
	[0 ... NR_SYSCALL - 1] = sys_null_placeholder,
	[KMK_SYS_poweroff] = sys_poweroff,
};
