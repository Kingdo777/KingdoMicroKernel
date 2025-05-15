#ifndef LIB_PRINTK_H
#define LIB_PRINTK_H

#include <common/types.h>

typedef void (*graphic_putc_handler)(char c);
extern graphic_putc_handler graphic_putc;

void set_graphic_putc_handler(graphic_putc_handler f);
void printk(const char *fmt, ...);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#endif /* LIB_PRINTK_H */