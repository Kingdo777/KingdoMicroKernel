#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <common/types.h>

static inline void memcpy(void *dst, const void *src, size_t size)
{
	char *d = (char *)dst;
	const char *s = (char *)src;

	while (size--) {
		*d++ = *s++;
	}
}

static inline void memset(void *dst, char ch, size_t size)
{
	char *p = (char *)dst;

	while (size--) {
		*p++ = ch;
	}
}

static inline int strcmp(const char *src, const char *dst)
{
	while (*src && *dst) {
		if (*src == *dst) {
			src++;
			dst++;
			continue;
		}
		return *src - *dst;
	}
	if (!*src && !*dst)
		return 0;
	if (!*src)
		return -1;
	return 1;
}

static inline int strncmp(const char *src, const char *dst, size_t size)
{
	size_t i;

	for (i = 0; i < size; ++i) {
		if (src[i] == '\0' || src[i] != dst[i])
			return src[i] - dst[i];
	}

	return 0;
}

static inline size_t strlen(const char *src)
{
	size_t i = 0;

	while (*src++)
		i++;

	return i;
}

static inline int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *l = (const unsigned char *)s1, *r = (const unsigned char *)s2;
	for (; n && *l == *r; n--, l++, r++)
		;
	return n ? *l - *r : 0;
}

#endif /* COMMON_UTILS_H */
