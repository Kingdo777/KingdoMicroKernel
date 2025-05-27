#ifndef IRQ_TIMER_H
#define IRQ_TIMER_H

#include <common/types.h>
#include <common/list.h>
#include <common/lock.h>
#include <machine.h>

/* 每多少ms触发一次定时中断 */
#define TICK_MS 10

#define NS_IN_S (1000000000UL)
#define US_IN_S (1000000UL)
#define NS_IN_US (1000UL)
#define US_IN_MS (1000UL)

/* Per-core timer states */
struct time_state {
	/* The tick when the next timer irq will occur */
	u64 next_expire;
	/*
         * Record all sleepers on each core.
         * Threads in sleep_list are sorted by the time to wakeup.
         * It is better to use more efficient data structure (tree)
         */
	struct list_head sleep_list;
	/* Protect per core sleep_list */
	struct lock sleep_list_lock;
};

void timer_init(void);
void handle_timer_irq(void);

void plat_timer_init(void);
void plat_set_next_timer(u64 tick_delta);
void plat_set_init_timer();
u64 plat_get_mono_time(void);
u64 plat_get_current_tick(void);

#endif // IRQ_TIMER_H