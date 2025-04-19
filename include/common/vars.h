#ifndef COMMON_VARS_H
#define COMMON_VARS_H

/* Leave 8K space to per-CPU stack */
#define CPU_STACK_SIZE          4096
#define STACK_ALIGNMENT         16


#if __SIZEOF_POINTER__ == 4 /* 32-bit architecture */
#define KBASE		0xC0000000UL
#else /* 64-bit architecture */
#define KBASE		0xFFFFFF0000000000UL
#endif

#endif /* COMMON_VARS_H */