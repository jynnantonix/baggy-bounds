
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "address_constants.h"

#define NUM_BINS 32
#define SLOT_SIZE 16  // must be a power of two
#define FREE 0
#define USED 1

extern unsigned char* baggy_size_table;

struct list_node_t {
	struct list_node_t* prev;
	struct list_node_t* next;
	unsigned int is_dummy;
};
typedef struct list_node_t list_node_t;

inline unsigned int get_slot_id(char*);
inline void table_mark_free(char*, unsigned int);
inline unsigned char get_slot_metadata(int);
inline void set_slot_metadata(int, unsigned char);
inline unsigned char form_metadata(unsigned char, unsigned char);
inline unsigned char is_used(unsigned char);
inline unsigned char get_logsize(unsigned char);
inline unsigned char get_log2(unsigned int);

char* heap_start;
char* heap_end;
unsigned int heap_size;  // the size of the heap in bytes
list_node_t *dummy_first[NUM_BINS];
list_node_t *dummy_last[NUM_BINS];

inline unsigned int list_empty(unsigned int bin_id) {
	return dummy_first[bin_id]->next->is_dummy;
}

inline void list_append(list_node_t* ptr, unsigned int bin_id) {
	ptr->is_dummy = 0;
	ptr->prev = dummy_first[bin_id];
	ptr->next = dummy_first[bin_id]->next;
	dummy_first[bin_id]->next->prev = ptr;
	dummy_first[bin_id]->next = ptr;
}

inline void list_remove(list_node_t* ptr) {
	list_node_t* prev = ptr->prev;
	list_node_t* next = ptr->next;
	ptr->next->prev = prev;
	prev->next = next;
}

inline unsigned int get_slot_id(char* ptr) {
	return (((unsigned int)ptr) - ((unsigned int)heap_start)) / SLOT_SIZE;
}

inline void table_mark_free(char* ptr, unsigned int size) {
	unsigned int log2_size = get_log2(size);
	unsigned int slot_id = get_slot_id(ptr);
	char* after_last_slot = ptr + (size / sizeof(char));
	while (((unsigned int)heap_start) + slot_id * SLOT_SIZE < ((unsigned int)after_last_slot)) {
		set_slot_metadata(slot_id, form_metadata(log2_size, FREE));
		slot_id++;
	}
}

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

void test_lists() {
	assert(list_empty(0) == 1);
	list_node_t* A = (list_node_t*)sbrk(sizeof(list_node_t));
	list_node_t* B = (list_node_t*)sbrk(sizeof(list_node_t));
	list_node_t* C = (list_node_t*)sbrk(sizeof(list_node_t));
	list_append(A, 0);
	list_append(C, 0);
	list_append(B, 0);
	assert(dummy_first[0]->next == B);
	assert(dummy_first[0]->next->next == C);
	assert(dummy_first[0]->next->next->next == A);
	assert(dummy_first[0]->next->next->next->next = dummy_last[0]);
	assert(dummy_last[0]->prev == A);
	assert(dummy_last[0]->prev->prev == C);
	assert(dummy_last[0]->prev->prev->prev == B);
	assert(dummy_last[0]->prev->prev->prev->prev == dummy_first[0]);
	assert(list_empty(0) == 0);
	list_remove(C);
	assert(dummy_first[0]->next == B);
	assert(dummy_first[0]->next->next == A);
	assert(dummy_first[0]->next->next->next == dummy_last[0]);
	assert(dummy_last[0]->prev == A);
	assert(dummy_last[0]->prev->prev == B);
	assert(dummy_last[0]->prev->prev->prev == dummy_first[0]);
	list_node_t* D = (list_node_t*)sbrk(sizeof(list_node_t));
	list_append(D, 0);
	assert(dummy_first[0]->next == D);
	assert(dummy_first[0]->next->next == B);
	assert(dummy_first[0]->next->next->next == A);
	assert(dummy_first[0]->next->next->next->next == dummy_last[0]);
	list_remove(D); 
	list_remove(A);
	assert(list_empty(0) == 0);
	assert(dummy_first[0]->next == B);
	assert(dummy_first[0]->next->next == dummy_last[0]);
	assert(dummy_last[0]->prev == B);
	assert(dummy_last[0]->prev->prev == dummy_first[0]);
	assert(dummy_last[0]->prev->is_dummy == 0);
	list_remove(B);
	assert(dummy_first[0]->next == dummy_last[0]);
	assert(dummy_last[0]->prev == dummy_first[0]);
	assert(list_empty(0) == 1);
	printf("all list tests passed..\n");
	fflush(stdout);
}

void buddy_allocator_init() {
	// initialize the free lists
	unsigned int bin_id;
	for (bin_id = 0; bin_id < NUM_BINS; ++bin_id) {
		dummy_first[bin_id] = (list_node_t*)sbrk(sizeof(list_node_t));
		dummy_last[bin_id] = (list_node_t*)sbrk(sizeof(list_node_t));
		
		dummy_first[bin_id]->is_dummy = 1;
		dummy_first[bin_id]->prev = NULL;
		dummy_first[bin_id]->next = dummy_last[bin_id];
		dummy_last[bin_id]->is_dummy = 1;
		dummy_last[bin_id]->prev = dummy_first[bin_id];
		dummy_last[bin_id]->next = NULL;
	}
	heap_start = (char*)TABLE_END;
	heap_end = (char*)sbrk(0);  // allocated heap is [heap_start, heap_end)

	// align heap_end to a multiple of SLOT_SIZE
	unsigned int slot_size_reminder = SLOT_SIZE - ((unsigned int)heap_end) % SLOT_SIZE;
	if (slot_size_reminder < SLOT_SIZE) {
		char *unused = sbrk(slot_size_reminder);
		heap_end = (char*)sbrk(0);
	}
	heap_size = ((unsigned int)heap_end) - ((unsigned int)heap_start);
	assert(((unsigned int)heap_end) % SLOT_SIZE == 0);
	
	// update the baggy table so the data after TABLE_END and before the beginning
	// of the real heap can't be reused
	int slot_id = 0;
	while (((unsigned int)heap_start) + slot_id * SLOT_SIZE < ((unsigned int)heap_end)) {
		set_slot_metadata(slot_id, form_metadata(get_log2(SLOT_SIZE), USED));
		slot_id++;
	}
/*
	// increase the size of the heap to an exact power of 2
	while ((heap_size & (heap_size - 1)) != 0) {
		bin_id = 0;
		while ((heap_size & (1 << bin_id)) == 0) {
			bin_id++;
		}
		unsigned int size = 1 << bin_id;
		char* ptr = (char*)sbrk(size);
		if (size >= SLOT_SIZE) {
			table_mark_free(ptr, size);
			list_append(ptr, bin_id);
		} else {
			// throw away
		}
		heap_end += size;
		heap_size += size;
	}
	assert((heap_size & (heap_size - 1)) == 0);*/
}

void* malloc(size_t size) {
/*
	if (size == 0) {	
		return NULL;
	}
	unsigned int size_to_allocate = 1 << get_log2(size);
	if (size_to_allocate < SLOT_SIZE) {
		size_to_allocate = SLOT_SIZE;
	}
	unsigned char log2 = get_log2(size_to_allocate);
	char* ptr;
	int bin_id;
	for (bin_id = log2; bin_id < NUM_BINS && freelist_head[bin] == NULL; ++bin_id);
	if (bin_id < NUM_BINS) {
		// grab the first element from the list
		
		// TODO(aanastasov): actually grab the element from there
	} else (bin_id >= NUM_BINS) {
		// try doubling the heap until allocate the required block
		unsigned int size_allocated;
		do {
			unsigned int heapsize_log2 = get_log2(heap_size);
			assert(get_log2(heap_size + 1) != heapsize_log2);  // the size of the heap must be a power of 2
			size_allocated = heap_size;
			bin_id = heapsize_log2;
			// TODO(aanastasov): Consider the case of not being able to allocate memory, and return NULL.
			ptr = (char*)sbrk(heap_size);
			table_mark_free(ptr, heap_size);
			list_append(ptr, bin_id);
			heap_end += heap_size;
			heap_size *= 2;
		} while (size_to_allocate > size_allocated);
	}

	assert(ptr != NULL);
	// if too big of a block, split
	while ((1 << (bin_id - 1)) >= size_to_allocate) {
		char* right_half_ptr = ptr + (1 << (bin_id - 1));
		table_mark_free(right_half_ptr, 1 << (bin_id - 1));
		list_append(right_half_ptr, bin_id - 1);
		bin_id--;
	}
*/	
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
