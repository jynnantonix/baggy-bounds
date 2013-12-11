#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *baggy_size_table;

char *baggy_strcat(char *destination, const char *source)
{
	size_t source_len, dest_len, alloc_size, log_size;
	intptr_t base_ptr, dest_ptr, offset;

	source_len = strlen(source);
	dest_len = strlen(destination);
	dest_ptr = (intptr_t)destination;
	log_size = (size_t)baggy_size_table[dest_ptr>>4];
	alloc_size = 1 << log_size;
	base_ptr = (dest_ptr & (~0xf));
	offset = dest_ptr - base_ptr;

	if (dest_len + source_len + offset > alloc_size) {
		puts("Baggy libc segmentation fault\n");
		exit(EXIT_FAILURE);
	}

	return strcat(destination, source);
}

char *baggy_strcpy(char *destination, const char *source)
{
	size_t source_len, alloc_size, log_size;
	intptr_t base_ptr, dest_ptr, offset;

	source_len = strlen(source);
	dest_ptr = (intptr_t)destination;
	log_size = baggy_size_table[dest_ptr>>4];
	alloc_size = 1 << log_size;
	base_ptr = (dest_ptr & (~0xf));
	offset = dest_ptr - base_ptr;

	if (source_len + offset > alloc_size) {
		puts("Baggy libc segmentation fault\n");
		exit(EXIT_FAILURE);
	}

	return strcpy(destination, source);
}

int baggy_sprintf (char *str, const char *format, ... )
{
	va_list argptr;
	size_t alloc_size, log_size;
	intptr_t base_ptr, dest_ptr, offset;
	int ret;

	va_start(argptr, format);
	ret = vsprintf(str, format, argptr);
	va_end(argptr);

	dest_ptr = (intptr_t)str;
	log_size = baggy_size_table[dest_ptr>>4];
	alloc_size = 1 << log_size;
	base_ptr = (dest_ptr & (~0xf));
	offset = dest_ptr = base_ptr;

	if (ret + offset > alloc_size) {
		puts("Baggy libc segmentation fault\n");
		exit(EXIT_FAILURE);
	}

	return ret;
}
