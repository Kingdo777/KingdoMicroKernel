#ifndef COMMON_LOCK_H
#define COMMON_LOCK_H

#include <arch/sync.h>
#include <common/types.h>

struct lock {
	volatile int lock;
};

#define lock_init(lock) spin_lock_init((spinlock_t *)lock)
#define lock(lock) spin_lock((spinlock_t *)lock)
#define unlock(lock) spin_unlock((spinlock_t *)lock)

/* Global locks */
extern struct lock big_kernel_lock;

#endif /* COMMON_LOCK_H */