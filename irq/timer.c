#include <irq/irq.h>
#include <irq/timer.h>
#include <arch/machine/smp.h>
#include <common/kprint.h>
#include <common/list.h>
#include <common/lock.h>

struct time_state time_states[PLAT_CPU_NUM];

void timer_init(void)
{
	int i;

	if (smp_get_cpu_id() == 0) {
		for (i = 0; i < PLAT_CPU_NUM; i++) {
			init_list_head(&time_states[i].sleep_list);
			lock_init(&time_states[i].sleep_list_lock);
		}
	}

	/* Per-core timer init */
	plat_timer_init();
}

void handle_timer_irq(void)
{
	plat_set_init_timer();
}