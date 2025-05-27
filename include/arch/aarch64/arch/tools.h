#ifndef ARCH_AARCH64_ARCH_TOOLS_H
#define ARCH_AARCH64_ARCH_TOOLS_H

void flush_dcache_area(unsigned long addr, unsigned long size);
void enable_irq(void);
void disable_irq(void);
void enable_uart_irq(int irqno);
void uart_irq_handler(void);
void put32(unsigned long addr, unsigned int data);
unsigned int get32(unsigned long addr);

#endif /* ARCH_AARCH64_ARCH_TOOLS_H */