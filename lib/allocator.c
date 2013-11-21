
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "address_constants.h"

#define NUM_BINS 32
#define SLOT_SIZE 16  // must be a power of two
#define USED 1
#define FREE 0

extern unsigned char* baggy_size_table;

struct list_node_t {
	struct list_node_t* next;
};
typedef struct list_node_t list_node_t;

unsigned int heap_size;  // the size of the heap in bytes
list_node_t* freelist_head[NUM_BINS]; // Pointers to the heads of the free lists for each power of 2.

inline unsigned char get_slot_metadata(int slot_id) {
	return (unsigned char)(*(baggy_size_table + slot_id * (SLOT_SIZE / sizeof(unsigned char))));
}

inline void set_slot_metadata(int slot_id, unsigned char value) {
	unsigned char* slot_entry_metadata = baggy_size_table + 
		slot_id * (SLOT_SIZE / sizeof(unsigned char));
	*slot_entry_metadata = value;
}

inline unsigned char form_metadata(unsigned char logsize, unsigned char is_used) {
	return (logsize << 1) | is_used;
}

inline unsigned char is_used(unsigned char metadata) {
	return metadata & 1;
}

inline unsigned char get_logsize(unsigned char metadata) {
	return metadata >> 1;
}

// returns smallest x such that 2^x >= size
// requires size > 0
inline unsigned char get_log2(unsigned int size) {
	unsigned char res = 0;
	size--;
	while (size > 0) {
		size >>= 1;
		++res;
	}
	return res;
}

void buddy_allocator_init() {
	char* heap_start = (char*)TABLE_END;
	char* heap_end = (char*)sbrk(0);  // allocated heap is [heap_start, heap_end)
	
	// align heap_end to a multiple of SLOT_SIZE
	unsigned int slot_size_reminder = SLOT_SIZE - ((unsigned int)heap_end) % SLOT_SIZE;
	if (slot_size_reminder < SLOT_SIZE) {
		char *unused = sbrk(slot_size_reminder);
		heap_end = (char*)sbrk(0);
	}
	heap_size = ((unsigned int)heap_end) - ((unsigned int)heap_start);
	assert(((unsigned int)heap_end) % SLOT_SIZE == 0);

	// update the baggy table to have the data after TABLE_END and before the beginning
	// of the actual heap as already in use
	int slot_id = 0;
	while (((unsigned int)heap_start) + slot_id * SLOT_SIZE <= ((unsigned int)heap_end)) {
		set_slot_metadata(slot_id, form_metadata(get_log2(SLOT_SIZE), USED));
		slot_id++;
	}

	int bin_id;
	for (bin_id = 0; bin_id < NUM_BINS; ++bin_id) {
		freelist_head[bin_id] = NULL;
	}
}

void* malloc(size_t size) {
	// check if the bin of size round(log_2(size)) is free
	// and grab an element from it, if possible
	// otherwise iterate over the free lists with bigger sizes
	// until find the smallest one that contains a free block
	// and split that block into blocks of powers of two such that the smallest
	// piece is of size log_2(size), and then populate the other free lists with
	// the pieces that are left unused
	// if this doesn't work, then we have to double the size of the heap, and apply
	// the same thing as above where we get a bigger power of two than the one we 
	// need and continually split the blocks
	// need to take care of putting the right information in the baggy_size_table
}

void* realloc(void* ptr, size_t size) {
	// go with the easiest first implementation in the beginning
}

void free(void* ptr) {
	// free don't do anything in the beginning, just add the pointer to the particular
	// bin; however the next revision would need to allow for coalescing blocks that
	// are both free
}
