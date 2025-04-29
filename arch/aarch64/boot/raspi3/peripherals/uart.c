#include <io/uart.h>
#include "uart.h"

#if USE_mini_uart == 1

void early_uart_init(void)
{
	unsigned int ra;

	ra = early_get32(GPFSEL1);
	ra &= ~(7 << 12);
	ra |= 2 << 12;
	ra &= ~(7 << 15);
	ra |= 2 << 15;
	early_put32(GPFSEL1, ra);

	early_put32(GPPUD, 0);
	delay(150);
	early_put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	early_put32(GPPUDCLK0, 0);

	early_put32(AUX_ENABLES, 1);
	early_put32(AUX_MU_IER_REG, 0);
	early_put32(AUX_MU_CNTL_REG, 0);
	early_put32(AUX_MU_IER_REG, 0);
	early_put32(AUX_MU_LCR_REG, 3);
	early_put32(AUX_MU_MCR_REG, 0);
	early_put32(AUX_MU_BAUD_REG, 270);

	early_put32(AUX_MU_CNTL_REG, 3);
}

static unsigned int early_uart_lsr(void)
{
	return early_get32(AUX_MU_LSR_REG);
}

static void early_uart_send(unsigned int c)
{
	while (1) {
		if (early_uart_lsr() & 0x20)
			break;
	}
	early_put32(AUX_MU_IO_REG, c);
}

#else /* PL011 */

void early_uart_init(void)
{
	unsigned int ra;

	ra = early_get32(GPFSEL1);

	/* Set GPIO14 as function 0. */
	ra &= ~(7 << 12);
	ra |= 4 << 12;
	/* Set GPIO15 as function 0. */
	ra &= ~(7 << 15);
	ra |= 4 << 15;

	early_put32(GPFSEL1, ra);
	early_put32(GPPUD, 0);
	delay(150);
	early_put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	early_put32(GPPUDCLK0, 0);

	/* Close serial briefly. */
	early_put32(RASPI3_PL011_CR, 0);
	/* Set baud rate as 115200. */
	early_put32(RASPI3_PL011_IBRD, 26);
	early_put32(RASPI3_PL011_FBRD, 3);
	/* Enable FIFO. */
	early_put32(RASPI3_PL011_LCRH, (1 << 4) | (3 << 5));
	/* Inhibit interrupt. */
	early_put32(RASPI3_PL011_IMSC, 0);
	/* Enable serial to send or receive data. */
	early_put32(RASPI3_PL011_CR, 1 | (1 << 8) | (1 << 9));
}

static unsigned int early_uart_fr(void)
{
	return early_get32(RASPI3_PL011_FR);
}

static void early_uart_send(unsigned int c)
{
	/* Check if the send fifo is full. */
	while (early_uart_fr() & (1 << 5))
		;
	early_put32(RASPI3_PL011_DR, c);
}

#endif

void uart_send_string(char *str)
{
	while (*str) {
		early_uart_send(*str);
		str++;
	}
}
