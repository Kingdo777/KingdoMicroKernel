#ifndef IO_UART_H
#define IO_UART_H

#define NB_UART_NRET        ((char) -1)

void uart_init(void);
void uart_send(char c);
char uart_recv(void);
char nb_uart_recv(void); //Non-blocking receive

#endif /* IO_UART_H */