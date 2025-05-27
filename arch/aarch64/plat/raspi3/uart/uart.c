#include <irq/irq.h>
#include <io/uart.h>
#include "uart.h"

static inline void delay(unsigned int cnt)
{
	while (cnt != 0)
		cnt--;
}

#if USE_mini_uart == 1

void uart_init(void)
{
	unsigned int ra;

	ra = get32(GPFSEL1);
	ra &= ~(7 << 12);
	ra |= 2 << 12;
	ra &= ~(7 << 15);
	ra |= 2 << 15;
	put32(GPFSEL1, ra);

	put32(GPPUD, 0);
	delay(150);
	put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	put32(GPPUDCLK0, 0);

	put32(AUX_ENABLES, 1);
	put32(AUX_MU_IER_REG, 0);
	put32(AUX_MU_CNTL_REG, 0);
	put32(AUX_MU_IER_REG, 0);
	put32(AUX_MU_LCR_REG, 3);
	put32(AUX_MU_MCR_REG, 0);
	put32(AUX_MU_BAUD_REG, 270);

	put32(AUX_MU_CNTL_REG, 3);

	/* Clear the screen */
	uart_send(12);
	uart_send(27);
	uart_send('[');
	uart_send('2');
	uart_send('J');
}

unsigned int uart_lsr(void)
{
	return get32(AUX_MU_LSR_REG);
}

char uart_recv(void)
{
	while (1) {
		if (uart_lsr() & 0x01)
			break;
	}

	return (char)(get32(AUX_MU_IO_REG) & 0xFF);
}

char nb_uart_recv(void)
{
	if (uart_lsr() & 0x01)
		return (char)((get32(AUX_MU_IO_REG) & 0xFF));
	else
		return NB_UART_NRET;
}

void uart_send(char c)
{
	while (1) {
		if (uart_lsr() & 0x20)
			break;
	}
	put32(AUX_MU_IO_REG, (unsigned int)c);
}

#else /* PL011 */

#define UART_BUF_LEN 16
typedef struct uart_buffer {
	char buffer[UART_BUF_LEN];
	int read_pos;
	int put_pos;
} uart_buffer_t;

uart_buffer_t pl011_buffer;

/**
 * @brief 配置 PL011 UART 控制器，使其在接收到数据时触发中断（IRQ）
 */
void enable_uart_irq(int irq_num)
{
	/* 清除未处理中断（Pending IRQ） */
	put32(RASPI3_PL011_ICR, 0x7ff);
	/* 写入 1 << 4（即 0x10）启用 ​​接收中断（RXIM）​​，表示当 UART 接收缓冲区有数据时触发中断 */
	put32(RASPI3_PL011_IMSC, 1 << 4);
	/* 配置 FIFO 触发阈值，接收2字节或FIFO​​完全为空且新数据到达后触发中断*/
	put32(RASPI3_PL011_IFLS, 0);

	/* 初始化一个环形缓冲区（pl011_buffer），用于存储 UART 接收到的数据 */
	pl011_buffer.read_pos = 0;
	pl011_buffer.put_pos = 0;

	plat_enable_irqno(irq_num);
}
/**
 * @brief PL011 UART 中断处理函数
 * 	  当 PL011 UART 接收到数据时，硬件触发中断，CPU 跳转到此函数处理数据
 */
void uart_irq_handler(void)
{
	char input;

	/* 读取数据寄存器，保留低8位 */
	input = get32(RASPI3_PL011_DR) & 0xFF;
	uart_send(input); // 数据回显（Echo）

	/* 读取 DR 会自动清除接收中断标志 (无需手动写 ICR) */
	// put32(RASPI3_PL011_ICR, 0x7ff);

	pl011_buffer.buffer[pl011_buffer.put_pos] = input;

	/*
	 * The internal fifo buffer is a ring buffer.
	 * Here is updating the put_pos.
	 */
	/* 数据存入环形缓冲区​ */
	pl011_buffer.put_pos += 1;
	if (pl011_buffer.put_pos == UART_BUF_LEN)
		pl011_buffer.put_pos = 0;

	/* 处理缓冲区溢出​, 丢弃最旧的数据（read_pos 后移），腾出空间 */
	if (pl011_buffer.read_pos == pl011_buffer.put_pos) {
		pl011_buffer.read_pos += 1;
		if (pl011_buffer.read_pos == UART_BUF_LEN)
			pl011_buffer.read_pos = 0;
	}
}

void uart_init(void)
{
	unsigned int ra;

	ra = get32(GPFSEL1);

	/* Set GPIO14 as function 0. */
	ra &= ~(7 << 12);
	ra |= 4 << 12;
	/* Set GPIO15 as function 0. */
	ra &= ~(7 << 15);
	ra |= 4 << 15;

	put32(GPFSEL1, ra);
	put32(GPPUD, 0);
	delay(150);
	put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	put32(GPPUDCLK0, 0);

	/* Close serial briefly. */
	put32(RASPI3_PL011_CR, 0);
	/* Set baud rate as 115200. */
	put32(RASPI3_PL011_IBRD, 26);
	put32(RASPI3_PL011_FBRD, 3);
	/* Enable FIFO. */
	//put32(RASPI3_PL011_LCRH, (1 << 4) | (3 << 5));
	/* Disable FIFO. */
	put32(RASPI3_PL011_LCRH, (0 << 4) | (3 << 5));
	/* Inhibit interrupt. */
	put32(RASPI3_PL011_IMSC, 0);
	/* Enable serial to send or receive data. */
	put32(RASPI3_PL011_CR, 1 | (1 << 8) | (1 << 9));

	/* Clear the screen */
	uart_send(12);
	uart_send(27);
	uart_send('[');
	uart_send('2');
	uart_send('J');
}

unsigned int uart_fr(void)
{
	return get32(RASPI3_PL011_FR);
}

char uart_recv(void)
{
	/* Check if the fifo is empty. */
	while (uart_fr() & (1 << 4))
		;

	return (char)(get32(RASPI3_PL011_DR) & 0xFF);
}

char nb_uart_recv(void)
{
	char ch;

	if (pl011_buffer.read_pos == pl011_buffer.put_pos)
		return NB_UART_NRET;

	ch = pl011_buffer.buffer[pl011_buffer.read_pos];

	pl011_buffer.read_pos += 1;
	if (pl011_buffer.read_pos == UART_BUF_LEN)
		pl011_buffer.read_pos = 0;

	return ch;
}

void uart_send(char c)
{
	/* Check if the send fifo is full. */
	while (uart_fr() & (1 << 5))
		;
	put32(RASPI3_PL011_DR, (unsigned int)c);
}

#endif