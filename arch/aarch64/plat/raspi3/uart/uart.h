#ifndef UART_H
#define UART_H

#include <common/vars.h>

/* This peripheral mapped offset is specific to BCM2837 */
#define PHYSADDR_OFFSET (KBASE + 0x3F000000UL)

/* BCM2835 and BCM2837 define the same offsets */
#define GPFSEL1 (PHYSADDR_OFFSET + 0x00200004)
#define GPSET0 (PHYSADDR_OFFSET + 0x0020001C)
#define GPCLR0 (PHYSADDR_OFFSET + 0x00200028)
#define GPPUD (PHYSADDR_OFFSET + 0x00200094)
#define GPPUDCLK0 (PHYSADDR_OFFSET + 0x00200098)

/* mini-UART */
#define AUX_ENABLES (PHYSADDR_OFFSET + 0x00215004)
#define AUX_MU_IO_REG (PHYSADDR_OFFSET + 0x00215040)
#define AUX_MU_IER_REG (PHYSADDR_OFFSET + 0x00215044)
#define AUX_MU_IIR_REG (PHYSADDR_OFFSET + 0x00215048)
#define AUX_MU_LCR_REG (PHYSADDR_OFFSET + 0x0021504C)
#define AUX_MU_MCR_REG (PHYSADDR_OFFSET + 0x00215050)
#define AUX_MU_LSR_REG (PHYSADDR_OFFSET + 0x00215054)
#define AUX_MU_MSR_REG (PHYSADDR_OFFSET + 0x00215058)
#define AUX_MU_SCRATCH (PHYSADDR_OFFSET + 0x0021505C)
#define AUX_MU_CNTL_REG (PHYSADDR_OFFSET + 0x00215060)
#define AUX_MU_STAT_REG (PHYSADDR_OFFSET + 0x00215064)
#define AUX_MU_BAUD_REG (PHYSADDR_OFFSET + 0x00215068)

/* PL011 */
#define RASPI3_PL011_BASE (PHYSADDR_OFFSET + 0x201000)
#define RASPI3_PL011_DR (RASPI3_PL011_BASE + 0x00)
#define RASPI3_PL011_FR (RASPI3_PL011_BASE + 0x18)
#define RASPI3_PL011_IBRD (RASPI3_PL011_BASE + 0x24)
#define RASPI3_PL011_FBRD (RASPI3_PL011_BASE + 0x28)
#define RASPI3_PL011_LCRH (RASPI3_PL011_BASE + 0x2C)
#define RASPI3_PL011_CR (RASPI3_PL011_BASE + 0x30)
#define RASPI3_PL011_IFLS (RASPI3_PL011_BASE + 0x34)
#define RASPI3_PL011_IMSC (RASPI3_PL011_BASE + 0x38)
#define RASPI3_PL011_RIS (RASPI3_PL011_BASE + 0x3C)
#define RASPI3_PL011_MIS (RASPI3_PL011_BASE + 0x40)
#define RASPI3_PL011_ICR (RASPI3_PL011_BASE + 0x44)

void put32(unsigned long int addr, unsigned int ch);
unsigned int get32(unsigned long int addr);

#endif /* UART_H */