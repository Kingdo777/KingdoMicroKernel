#ifndef ARCH_AARCH64_ARCH_BOOT_H
#define ARCH_AARCH64_ARCH_BOOT_H

#define PHYS_MEM_END     0x3F000000UL

extern char img_end;
extern unsigned long boot_ttbr1_l0[];

#endif /* ARCH_AARCH64_ARCH_BOOT_H */
