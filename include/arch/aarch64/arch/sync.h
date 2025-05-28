#ifndef ARCH_AARCH64_ARCH_SYNC_H
#define ARCH_AARCH64_ARCH_SYNC_H

#include <common/types.h>

typedef struct {
	volatile int lock;
} spinlock_t;

#define SPINLOCK_INIT() \
	{               \
		0       \
	}
#define DEFINE_SPINLOCK(x) spinlock_t x = SPINLOCK_INIT()

#define sev() asm volatile("sev" : : : "memory")
#define wfe() asm volatile("wfe" : : : "memory")
#define wfi() asm volatile("wfi" : : : "memory")

#define isb() asm volatile("isb" : : : "memory")
#define dmb(opt) asm volatile("dmb " #opt : : : "memory")
#define dsb(opt) asm volatile("dsb " #opt : : : "memory")

#define mb() dsb(sy)
#define rmb() dsb(ld)
#define wmb() dsb(st)

#define smp_mb() dmb(ish)
#define smp_rmb() dmb(ishld)
#define smp_wmb() dmb(ishst)

#define dma_rmb() dmb(oshld)
#define dma_wmb() dmb(oshst)

#define ldar_32(ptr, value) asm volatile("ldar %w0, [%1]" : "=r"(value) : "r"(ptr))
#define stlr_32(ptr, value) asm volatile("stlr %w0, [%1]" : : "rZ"(value), "r"(ptr))

#define ldar_64(ptr, value) asm volatile("ldar %x0, [%1]" : "=r"(value) : "r"(ptr))
#define stlr_64(ptr, value) asm volatile("stlr %x0, [%1]" : : "rZ"(value), "r"(ptr))

#define __atomic_compare_exchange(ptr, compare, exchange, len, width)  \
	({                                                             \
		u##len oldval;                                         \
		u32 ret;                                               \
		asm volatile("1: ldaxr   %" #width "0, %2\n"           \
			     "   cmp     %" #width "0, %" #width "3\n" \
			     "   b.ne    2f\n"                         \
			     "   stlxr   %w1, %" #width "4, %2\n"      \
			     "   cbnz    %w1, 1b\n"                    \
			     "2:"                                      \
			     : "=&r"(oldval), "=&r"(ret), "+Q"(*(ptr)) \
			     : "r"(compare), "r"(exchange));           \
		oldval;                                                \
	})
#define atomic_compare_exchange_64(ptr, compare, exchange) __atomic_compare_exchange(ptr, compare, exchange, 64, x)
#define atomic_compare_exchange_32(ptr, compare, exchange) __atomic_compare_exchange(ptr, compare, exchange, 32, w)
/*
 * 原子比较并交换（Compare-and-Swap, CAS）操作，对比值compare和ptr指向的值，
 * 如果相等，则将ptr指向的值替换为exchange，并返回ptr原来的值。
 * 如果不相等，则不做任何操作，返回ptr指向的值。
 */
#define atomic_cmpxchg_32 atomic_compare_exchange_32
#define atomic_cmpxchg_64 atomic_compare_exchange_64

/*
 * 原子交换操作，将ptr指向的值替换为exchange，并返回ptr原来的值。
 * 该操作是原子的，保证在多线程环境下不会出现数据竞争。
 */
static inline s64 atomic_exchange_64(s64 *ptr, s64 exchange)
{
	s64 oldval;
	s32 ret;
	asm volatile("1: ldaxr   %x0, %2\n"
		     "   stlxr   %w1, %x3, %2\n"
		     "   cbnz    %w1, 1b\n"
		     "2:"
		     : "=&r"(oldval), "=&r"(ret), "+Q"(*ptr)
		     : "r"(exchange));
	return oldval;
}

#define __atomic_fetch_op(ptr, val, len, width, op)                                    \
	({                                                                             \
		u##len oldval, newval;                                                 \
		u32 ret;                                                               \
		asm volatile("1: ldaxr   %" #width "0, %3\n"                           \
			     "   " #op "   %" #width "1, %" #width "0, %" #width "4\n" \
			     "   stlxr   %w2, %" #width "1, %3\n"                      \
			     "   cbnz    %w2, 1b\n"                                    \
			     "2:"                                                      \
			     : "=&r"(oldval), "=&r"(newval), "=&r"(ret), "+Q"(*(ptr))  \
			     : "r"(val));                                              \
		oldval;                                                                \
	})

/*
 * 原子加/减法操作，将ptr指向的值加上/减去val，并返回ptr原来的值。
 * 该操作是原子的，保证在多线程环境下不会出现数据竞争。
 */
#define atomic_fetch_sub_32(ptr, val) __atomic_fetch_op(ptr, val, 32, w, sub)
#define atomic_fetch_sub_64(ptr, val) __atomic_fetch_op(ptr, val, 64, x, sub)
#define atomic_fetch_add_32(ptr, val) __atomic_fetch_op(ptr, val, 32, w, add)
#define atomic_fetch_add_64(ptr, val) __atomic_fetch_op(ptr, val, 64, x, add)

static inline void spin_lock_init(spinlock_t *lock)
{
	lock->lock = 0;
}

static inline void spin_lock(spinlock_t *lock)
{
	int tmp;
	asm volatile("1:     ldaxr   %w0, [%1]\n" // acquire‑load，无需显式添加内存屏障
		     "       cbnz    %w0, 2f\n" // 已被占用，跳到 2
		     "       stxr    %w0, %w2, [%1]\n" // CAS 上锁，失败重试
		     "       cbnz    %w0, 1b\n" // 失败，重试
		     "       b       3f\n" // 成功，跳出
		     "2:     wfe\n" // 等待 event（低功耗挂起）
		     "       b       1b\n" // 收到自动事件／中断后重试
		     "3:\n"
		     : "=&r"(tmp)
		     : "r"(&lock->lock), "r"(1)
		     : "memory");
}

static inline void spin_unlock(spinlock_t *lock)
{
	asm volatile("   stlr    wzr, [%0]\n" // release‑store：写 0，并自动发事件唤醒（sev）
		     :
		     : "r"(&lock->lock)
		     : "memory");
}

#endif /* ARCH_AARCH64_ARCH_SYNC_H */
