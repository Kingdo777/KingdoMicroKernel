#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef long long s64;
typedef int s32;
typedef short s16;
typedef signed char s8;

#define NULL ((void *)0)

typedef char bool;
#define true (1)
#define false (0)

#if __SIZEOF_POINTER__ == 4
typedef u32 paddr_t;
typedef u32 vaddr_t;
typedef u32 atomic_cnt;
typedef u32 clockid_t;
typedef u32 size_t;
#else
typedef u64 paddr_t;
typedef u64 vaddr_t;
typedef u64 atomic_cnt;
typedef u64 clockid_t;
typedef u64 size_t;
#endif

/** musl off_t is always 64-bit on any architectures */
typedef s64 off_t;
/** musl-1.2.0 enforces 64bit time_t on any architectures */
typedef u64 time_t;

/* The definition of timespec should be consistent with that in libc */
struct timespec {
	time_t tv_sec;
#ifdef BIG_ENDIAN
	int : 8 * (sizeof(time_t) - sizeof(long));
	long tv_nsec;
#else
	long tv_nsec;
	int : 8 * (sizeof(time_t) - sizeof(long));
#endif
};

#endif /* COMMON_TYPES_H */