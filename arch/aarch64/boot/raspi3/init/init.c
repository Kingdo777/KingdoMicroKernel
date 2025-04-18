#include "boot.h"
#include "consts.h"

char boot_cpu_stack[PLAT_CPU_NUMBER][INIT_STACK_SIZE] ALIGN(4096);

void early_uart_init(void);
void uart_send_string(char *str);

void init(void)
{
    early_uart_init();
    uart_send_string("Hello KMK!\n");
    while (1);
}