#ifndef COMMON_KPRINT_H
#define COMMON_KPRINT_H

#include <lib/printk.h>

#define WARNING 0
#define INFO 1
#define DEBUG 2

/* LOG_LEVEL is INFO by default */

#define kerror(fmt, ...)                                                      \
	printk("[ERRO] file: %s:%u " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
	while (1)                                                             \
		;

#if LOG_LEVEL >= WARNING
#define kwarn(fmt, ...) \
	printk("[WARN] file: %s:%u " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define kwarn(fmt, ...)
#endif

#if LOG_LEVEL >= INFO
#define kinfo(fmt, ...) printk("[INFO] " fmt, ##__VA_ARGS__)
#else
#define kinfo(fmt, ...)
#endif

#if LOG_LEVEL >= DEBUG
#define kdebug(fmt, ...) printk("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define kdebug(fmt, ...)
#endif

#endif /* COMMON_KPRINT_H */