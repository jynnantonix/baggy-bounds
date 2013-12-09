#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *baggy_size_table;

char *baggy_strcat(char *destination, const char *source)
{
	size_t source_len, dest_len, alloc_size;
	intptr_t dest_ptr;

	source_len = strlen(source);
	dest_len = strlen(destination);
	dest_ptr = (intptr_t)destination;
	alloc_size = (size_t)(1 << (int)(baggy_size_table[dest_ptr>>4]));

	if (dest_len + source_len > alloc_size) {
		puts("Baggy libc segmentation fault\n");
		exit(EXIT_FAILURE);
	}

	return strcat(destination, source);
}

char *baggy_strcpy(char *destination, const char *source)
{
	size_t source_len, alloc_size;
	intptr_t dest_ptr;

	source_len = strlen(source);
	dest_ptr = (intptr_t)destination;
	alloc_size = (size_t)(1 << (int)(baggy_size_table[dest_ptr>>4]));

	if (source_len > alloc_size) {
		puts("Baggy libc segmentation fault\n");
		exit(EXIT_FAILURE);
	}

	return strcpy(destination, source);
}
