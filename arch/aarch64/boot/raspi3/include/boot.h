#ifndef BOOT_H
#define BOOT_H

/* defined in tools.S */
void el1_mmu_activate(void);
/* defined in mmu.c */
void init_kernel_pt(void);

/* defined in arch/aarch64/head.S */
void start_kernel(void *boot_flag);
void secondary_cpu_boot(int cpuid);

extern char _bss_start;
extern char _bss_end;

extern char img_start;
extern char init_end;

extern char _text_start;
extern char _text_end;

#define PLAT_CPU_NUMBER		4

#define ALIGN(n)		__attribute__((__aligned__(n)))

#endif /* BOOT_H */
