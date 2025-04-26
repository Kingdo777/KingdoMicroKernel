#include <common/types.h>
#include <common/macro.h>
#include <io/uart.h>

static void uint_to_str(unsigned long num, char *buffer, int base,
			int uppercase)
{
	const char *digits = uppercase ? "0123456789ABCDEF" :
					 "0123456789abcdef";
	int i = 0;

	if (num == 0) {
		buffer[i++] = '0';
	} else {
		while (num != 0) {
			unsigned long rem = num % base;
			buffer[i++] = digits[rem];
			num = num / base;
		}
	}

	// 反转字符串
	int left = 0, right = i - 1;
	while (left < right) {
		char tmp = buffer[left];
		buffer[left] = buffer[right];
		buffer[right] = tmp;
		left++;
		right--;
	}
	buffer[i] = '\0';
}

// 有符号数字转字符串（十进制专用）
static void int_to_str(long num, char *buffer)
{
	int i = 0;
	int is_negative = 0;

	if (num < 0) {
		is_negative = 1;
		num = -num;
	}

	if (num == 0) {
		buffer[i++] = '0';
	} else {
		while (num != 0) {
			buffer[i++] = (num % 10) + '0';
			num /= 10;
		}
	}

	if (is_negative)
		buffer[i++] = '-';

	buffer[i] = '\0';

	// 反转字符串
	int left = 0, right = i - 1;
	while (left < right) {
		char tmp = buffer[left];
		buffer[left] = buffer[right];
		buffer[right] = tmp;
		left++;
		right--;
	}
}

void printk(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	while (*fmt) {
		if (*fmt != '%') {
			uart_send(*fmt);
			fmt++;
			continue;
		}

		fmt++; // 跳过%
		switch (*fmt) {
		case '%':
			uart_send('%');
			break;

		case 'c': {
			int c = va_arg(args, int);
			uart_send(c);
			break;
		}

		case 's': {
			char *s = va_arg(args, char *);
			while (*s)
				uart_send(*s++);
			break;
		}

		case 'd': { // 32位有符号
			int num = va_arg(args, int);
			char buffer[24];
			int_to_str(num, buffer);
			for (char *p = buffer; *p; p++)
				uart_send(*p);
			break;
		}

		case 'u': { // 32位无符号
			unsigned int num = va_arg(args, unsigned int);
			char buffer[24];
			uint_to_str(num, buffer, 10, 0);
			for (char *p = buffer; *p; p++)
				uart_send(*p);
			break;
		}

		case 'p': { // 指针类型
			void *ptr = va_arg(args, void *);
			unsigned long num = (unsigned long)ptr;
			char buffer[24];
			uart_send('0');
			uart_send('x'); // 添加0x前缀
			uint_to_str(num, buffer, 16, 0);
			for (char *p = buffer; *p; p++)
				uart_send(*p);
			break;
		}

		case 'l': { // 长整型处理
			fmt++;
			switch (*fmt) {
			case 'd': { // 64位有符号
				long num = va_arg(args, long);
				char buffer[24];
				int_to_str(num, buffer);
				for (char *p = buffer; *p; p++)
					uart_send(*p);
				break;
			}

			case 'u': { // 64位无符号
				unsigned long num = va_arg(args, unsigned long);
				char buffer[24];
				uint_to_str(num, buffer, 10, 0);
				for (char *p = buffer; *p; p++)
					uart_send(*p);
				break;
			}

			case 'x': { // 64位十六进制
				unsigned long num = va_arg(args, unsigned long);
				char buffer[24];
				uint_to_str(num, buffer, 16, 0);
				for (char *p = buffer; *p; p++)
					uart_send(*p);
				break;
			}

			default:
				uart_send('l');
				uart_send(*fmt);
				break;
			}
			break;
		}

		default:
			uart_send('%');
			uart_send(*fmt);
			break;
		}
		fmt++;
	}

	va_end(args);
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char *ptr = buf;
	size_t remaining = size ? size - 1 : 0; // 保留终止符空间
	int total_len = 0;

	while (*fmt) {
		if (*fmt != '%') {
			// 直接复制普通字符
			if (remaining > 0) {
				*ptr++ = *fmt;
				remaining--;
			}
			total_len++;
			fmt++;
			continue;
		}

		// 处理格式说明符
		fmt++;
		switch (*fmt) {
		case '%': {
			if (remaining > 0) {
				*ptr++ = '%';
				remaining--;
			}
			total_len++;
			break;
		}

		case 'c': {
			int c = va_arg(args, int);
			if (remaining > 0) {
				*ptr++ = (char)c;
				remaining--;
			}
			total_len++;
			break;
		}

		case 's': {
			char *s = va_arg(args, char *);
			if (!s)
				s = "(null)";
			while (*s) {
				if (remaining > 0) {
					*ptr++ = *s;
					remaining--;
				}
				s++;
				total_len++;
			}
			break;
		}

		case 'd': {
			int num = va_arg(args, int);
			char tmp[24];
			int_to_str(num, tmp);
			for (char *p = tmp; *p; p++) {
				if (remaining > 0) {
					*ptr++ = *p;
					remaining--;
				}
				total_len++;
			}
			break;
		}

		case 'u': {
			unsigned int num = va_arg(args, unsigned int);
			char tmp[24];
			uint_to_str(num, tmp, 10, 0);
			for (char *p = tmp; *p; p++) {
				if (remaining > 0) {
					*ptr++ = *p;
					remaining--;
				}
				total_len++;
			}
			break;
		}

		case 'p': {
			void *addr = va_arg(args, void *);
			char tmp[24];
			uint_to_str((unsigned long)addr, tmp, 16, 0);
			// 添加 "0x" 前缀
			for (const char *pre = "0x"; *pre; pre++) {
				if (remaining > 0) {
					*ptr++ = *pre;
					remaining--;
				}
				total_len++;
			}
			for (char *p = tmp; *p; p++) {
				if (remaining > 0) {
					*ptr++ = *p;
					remaining--;
				}
				total_len++;
			}
			break;
		}

		case 'l': {
			fmt++; // 处理 long 类型
			switch (*fmt) {
			case 'd': {
				long num = va_arg(args, long);
				char tmp[24];
				int_to_str(num, tmp);
				for (char *p = tmp; *p; p++) {
					if (remaining > 0) {
						*ptr++ = *p;
						remaining--;
					}
					total_len++;
				}
				break;
			}

			case 'u': {
				unsigned long num = va_arg(args, unsigned long);
				char tmp[24];
				uint_to_str(num, tmp, 10, 0);
				for (char *p = tmp; *p; p++) {
					if (remaining > 0) {
						*ptr++ = *p;
						remaining--;
					}
					total_len++;
				}
				break;
			}

			case 'x': {
				unsigned long num = va_arg(args, unsigned long);
				char tmp[24];
				uint_to_str(num, tmp, 16, 0);
				for (char *p = tmp; *p; p++) {
					if (remaining > 0) {
						*ptr++ = *p;
						remaining--;
					}
					total_len++;
				}
				break;
			}

			default: // 无效格式
				if (remaining > 0) {
					*ptr++ = 'l';
					remaining--;
					total_len++;
				}
				fmt--; // 回退以处理未知字符
				break;
			}
			break;
		}

		default: { // 未知格式符
			if (remaining > 0) {
				*ptr++ = '%';
				*ptr++ = *fmt;
				remaining = remaining > 1 ? remaining - 2 : 0;
			}
			total_len += 2;
			break;
		}
		}
		fmt++;
	}

	// 确保终止符
	if (size > 0)
		*ptr = '\0';
	va_end(args);
	return total_len;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	return snprintf(buf, size, fmt, args);
}