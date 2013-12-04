#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SLOT_SIZE 16

extern unsigned char *baggy_size_table;

void *baggy_slowpath(void *buf, void *p)
{
	void *ret = NULL;
	intptr_t offset, alloc_size, diff, orig, newptr;
	unsigned char size;

	orig = (intptr_t)buf;
	newptr = (intptr_t)p;
	if ((orig & 0x80000000) != 0) {
		/* original pointer is out of bounds */
		orig &= 0x7fffffff;
		if ((orig & (SLOT_SIZE-1)) < (SLOT_SIZE>>1)) {
			orig -= SLOT_SIZE; /* bottom half of slot */
		} else {
			orig += SLOT_SIZE; /* top half of slot */
		}
		ret = baggy_slowpath((void *)orig, (void *)(newptr & 0x7fffffff));
	} else {
		/* get allocation size */
		offset = orig >> 4;
		size = baggy_size_table[offset];
		alloc_size = 1 << size;

		/* get start of allocation and calculate diff */
		orig = (orig >> size) << size;
		diff = newptr - orig;
		if (0 <= diff && diff < alloc_size) {
			ret = (void *)newptr;
		} else if (diff < (alloc_size + (SLOT_SIZE>>1)) && newptr >= (orig - (SLOT_SIZE>>1))) {
			ret = (void *)(newptr | 0x80000000);
		} else {
			printf("Baggy segmentation fault\n");
			exit(EXIT_FAILURE);
		}
	}

	return ret;
}
