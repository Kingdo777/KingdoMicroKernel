#ifndef ARCH_AARCH64_IRQ_IRQ_H
#define ARCH_AARCH64_IRQ_IRQ_H

#include <irq_num.h>

#define HANDLE_KERNEL	0
#define HANDLE_USER	1
extern unsigned char irq_handle_type[MAX_IRQ_NUM];

/* in arch/xxx/irq/irq_entry.c */
void arch_interrupt_init(void); 

/* in arch/xxx/plat/xxx/irq/irq.c */
void plat_interrupt_init(void);
void plat_handle_irq(void);
void plat_enable_irqno(int irq);
void plat_disable_irqno(int irq);

#endif /* ARCH_AARCH64_IRQ_IRQ_H */