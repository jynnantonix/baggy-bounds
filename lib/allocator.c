
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "address_constants.h"

#define NUM_BINS 32
#define SLOT_SIZE 16  // must be a power of two
#define FREE 0
#define USED 1

#define malloc(...) (USE_BUDDY_ALLOC)
#define free(...) (USE_BUDDY_FREE)
#define realloc(...) (USE_BUDDY_REALLOC)

extern unsigned char* baggy_size_table;

struct list_node_t {
	struct list_node_t* prev;
	struct list_node_t* next;
	unsigned int is_dummy;
};
typedef struct list_node_t list_node_t;

static inline unsigned int get_slot_id(char*);
static inline void table_mark(char*, unsigned int, unsigned char);
static inline unsigned char get_slot_metadata(int);
static inline void set_slot_metadata(unsigned int, unsigned char);
static inline unsigned char form_metadata(unsigned char, unsigned char);
static inline unsigned char is_used(unsigned char);
static inline unsigned char get_logsize(unsigned char);
static inline unsigned int get_log2(unsigned int);

void* buddy_malloc(size_t);
void* buddy_realloc(void*, size_t);
void buddy_free(void*);

char* heap_start;
char* heap_end;
unsigned int heap_size;  // the size of the heap in bytes
list_node_t *dummy_first[NUM_BINS];
list_node_t *dummy_last[NUM_BINS];

static inline unsigned int list_empty(unsigned int bin_id) {
	return dummy_first[bin_id]->next->is_dummy;
}

static inline void list_append(list_node_t* ptr, unsigned int bin_id) {
	ptr->is_dummy = 0;
	ptr->prev = dummy_first[bin_id];
	ptr->next = dummy_first[bin_id]->next;
	dummy_first[bin_id]->next->prev = ptr;
	dummy_first[bin_id]->next = ptr;
}

static inline void list_remove(list_node_t* ptr) {
	list_node_t* prev = ptr->prev;
	list_node_t* next = ptr->next;
	ptr->next->prev = prev;
	prev->next = next;
}

static inline unsigned int get_slot_id(char* ptr) {
	return (((unsigned int)ptr) - ((unsigned int)heap_start)) / SLOT_SIZE;
}

// requires size to be a power of two
static inline void table_mark(char* ptr, unsigned int size, unsigned char used) {
	assert((size & (size - 1)) == 0);
	unsigned int log2_size = get_log2(size);
	unsigned int slot_id = get_slot_id(ptr);
	unsigned int first_slot_id = get_slot_id(ptr);
	char* first_address_after_last_slot = ptr + (size / sizeof(char));
	while (((unsigned int)heap_start) + slot_id * SLOT_SIZE <
			((unsigned int)first_address_after_last_slot)) {
		// TODO(aanastasov): try to optimize by just setting the first slot,
		// not all of them
		set_slot_metadata(slot_id, form_metadata(log2_size, used));
		slot_id++;
	}
	assert(slot_id - first_slot_id == size / SLOT_SIZE);
}

static inline unsigned char get_slot_metadata(int slot_id) {
	return (unsigned char)baggy_size_table[slot_id];
}

static inline void set_slot_metadata(unsigned int slot_id, unsigned char value) {
	baggy_size_table[slot_id] = value;
}

static inline unsigned char form_metadata(unsigned char logsize, unsigned char is_used) {
	return logsize | (is_used << 7);
}

static inline unsigned char is_used(unsigned char metadata) {
	return (metadata & 128) >> 7;
}

static inline unsigned char get_logsize(unsigned char metadata) {
	return metadata & 127;
}

// returns smallest x such that 2^x >= size
// requires size > 0
static inline unsigned int get_log2(unsigned int size) {
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
	list_node_t* A = (list_node_t*)buddy_malloc(sizeof(list_node_t));
	list_node_t* B = (list_node_t*)buddy_malloc(sizeof(list_node_t));
	list_node_t* C = (list_node_t*)buddy_malloc(sizeof(list_node_t));
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
	list_node_t* D = (list_node_t*)buddy_malloc(sizeof(list_node_t));
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

void test_malloc() {
	int* a = (int*)buddy_malloc(sizeof(int));
	int* b = (int*)buddy_malloc(sizeof(int));
	*a = 5;
	*b = 7;
	*b = *a - 100;
	assert(*b == -95);
	assert(*a == 5);

	int* arr = (int*)buddy_malloc(sizeof(int) * 100);
	int i, j;
	for (i = 0; i < 128; ++i) {
		arr[i] = i;
	}
	for (i = 127; i >= 0; --i) {
		assert(arr[i] == i);
	}
	printf("basic malloc tests passed..\n");
	fflush(stdout);

	int* block[128];
	for (i = 0; i < 128; ++i) {
		block[i] = (int*)buddy_malloc(sizeof(int) * 100);
		for (j = 0; j < 128; ++j) {
			block[i][j] = j * j + 2 * j - 10;
		}
	}
	for (i = 0; i < 128; ++i) {
		for (j = 0; j < 128; ++j) {
			assert(block[i][j] == j * j + 2 * j - 10);
		}
	}
	printf("multiple allocations are powers of 2 malloc test passed..\n");
	fflush(stdout);

	int* segment[24];
	for (i = 0; i < 24; ++i) {
		segment[i] = (int*)buddy_malloc(sizeof(int) * (1 << i));
		for (j = 0; j < (1 << i); ++j) {
			segment[i][j] = j * j + 2 * j - 10;
		}
	}
	for (i = 0; i < 24; ++i)
		for (j = 0; j < (1 << i); ++j)
			assert(segment[i][j] == j * j + 2 * j - 10);
	printf("allocate big blocks malloc test passed..\n");
	fflush(stdout);
}

void test_free() {
	int *arr = (int*)buddy_malloc(sizeof(int) * (1 << 23));
	int i;
	for (i = 0; i < (1 << 23); ++i) {
		arr[i] = i * i + 2 * i - 30;
	}
	for (i = 0; i < (1 << 23); ++i) {
		assert(arr[i] == i * i + 2 * i - 30);
	}
	unsigned int old_heap_size = heap_size;
	buddy_free(arr);
	arr = (int*)buddy_malloc(sizeof(int) * (1 << 23));
	for (i = 0; i < (1 << 23); ++i) {
		arr[i] = i * i + 3 * i - 50;
	}
	for (i = 0; i < (1 << 23); ++i) {
		assert(arr[i] == i * i + 3 * i - 50);
	}
	// check that we are actually reusing the block, and not increasing
	// the size of the heap
	assert(heap_size == old_heap_size);
	printf("simple buddy_free test passed..\n");
	fflush(stdout);
}

void test_realloc() {
	int *arr = (int*)buddy_malloc(sizeof(int) * 1000);
	int i;
	for (i = 0; i < 1000; ++i) {
		arr[i] = i * i * i - 3 * i + 124;
	}
	arr = (int*)buddy_realloc(arr, sizeof(int) * 2000);
	assert(arr != NULL);
	for (i = 0; i < 1000; ++i) {
		assert(arr[i] == i * i * i - 3 * i + 124);
	}
	printf("simple buddy_realloc test passed..\n");
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
	heap_start = (char*)0;
	heap_end = (char*)sbrk(0);  // allocated heap is [heap_start, heap_end)

	// align heap_end to a multiple of SLOT_SIZE
	unsigned int slot_size_reminder = SLOT_SIZE - ((unsigned int)heap_end) % SLOT_SIZE;
	if (slot_size_reminder < SLOT_SIZE) {
		sbrk(slot_size_reminder);  // throw away
		heap_end = (char*)sbrk(0);
	}
	heap_size = ((unsigned int)heap_end) - ((unsigned int)heap_start);
	assert(((unsigned int)heap_end) % SLOT_SIZE == 0);

	// update the baggy table so the data after TABLE_END and before the beginning
	// of the real heap can't be reused
	unsigned slot_id = 0;
	while (((unsigned int)heap_start) + slot_id * SLOT_SIZE < ((unsigned int)heap_end)) {
		set_slot_metadata(slot_id, form_metadata(get_log2(SLOT_SIZE), USED));
		slot_id++;
	}
}

static inline char* increase_heap_size_and_get_ptr(unsigned int size_to_allocate,
		unsigned int* _bin_id) {
	unsigned int size_allocated;
	char* ptr;
	do {
		unsigned int bin_id = 0;
		while ((heap_size & (1 << bin_id)) == 0) {
			bin_id++;
		}
		// TODO(aanastasov): consider the case when sbrk fails
		size_allocated = 1 << bin_id;
		*_bin_id = bin_id;
		ptr = (char*)sbrk(size_allocated);
		if (size_allocated >= SLOT_SIZE) {
			table_mark(ptr, size_allocated, FREE);
			list_append((list_node_t*)ptr, bin_id);
		} else {
			// too small (can't fit one SLOT_SIZE entry), throw away
		}
		heap_end += size_allocated;
		heap_size += size_allocated;
		assert(heap_end == (char*)sbrk(0));
	} while (size_to_allocate > size_allocated);
	return ptr;
}

void* buddy_malloc(size_t size) {
	if (size == 0) {
		return NULL;
	}
	// allocate the smallest sufficient power of two block >= size
	unsigned int size_to_allocate = 1 << ((unsigned int)get_log2(size));
	if (size_to_allocate < SLOT_SIZE) {
		size_to_allocate = SLOT_SIZE;
	}
	char* ptr;
	unsigned char log2 = get_log2(size_to_allocate);
	unsigned int bin_id;
	// find first-fit block
	for (bin_id = log2; bin_id < NUM_BINS; ++bin_id) {
		if (dummy_first[bin_id]->next->is_dummy == 0)
			break;
	}
	if (bin_id < NUM_BINS) {
		// grab the first element from the list
		ptr = (void*)dummy_first[bin_id]->next;
	} else {
		// try increasing the size of the heap until can allocate the required block
		ptr = (void*)increase_heap_size_and_get_ptr(size_to_allocate, &bin_id);
	}

	assert(ptr != NULL);
	assert(!is_used(get_slot_metadata(get_slot_id(ptr))));
	assert(get_logsize(get_slot_metadata(get_slot_id(ptr))) == bin_id);

	// if the block is too big, split into pieces, and populate bins
	while ((1 << (bin_id - 1)) >= size_to_allocate) {
		char* right_half_ptr = ptr + (1 << (bin_id - 1));
		table_mark(right_half_ptr, 1 << (bin_id - 1), FREE);
		list_append((list_node_t*)right_half_ptr, bin_id - 1);
		assert(get_logsize(get_slot_metadata(get_slot_id(right_half_ptr))) ==
			bin_id - 1);
		assert(is_used(get_slot_metadata(get_slot_id(right_half_ptr))) == 0);
		bin_id--;
	}
	// mark as not free, and return
	table_mark(ptr, 1 << bin_id, USED);
	list_remove((list_node_t*)ptr);
	return (void*)ptr;
}

void* buddy_realloc(void* ptr, size_t size) {
	// TODO(aanastasov): Add optimization to realloc if the new block is shortened,
	// and remove copying data around
	void* newptr;
	size_t copy_size;
	
	newptr = buddy_malloc(size);
	if (newptr == NULL) {
		return NULL;
	}

	copy_size = 1 << get_logsize(get_slot_metadata(get_slot_id(ptr)));
	if (size < copy_size)
		copy_size = size;	

	memcpy(newptr, ptr, copy_size);

	buddy_free(ptr);
	return newptr;
}

void buddy_free(void* ptr) {
	unsigned int size = 1 << get_logsize(get_slot_metadata(get_slot_id(ptr)));
	unsigned int bin_id = get_log2(size);
	table_mark(ptr, size, FREE);
	list_append((list_node_t*)ptr, bin_id);

	unsigned char try_coalescing = 1;
	do {
		unsigned int address = (unsigned int)ptr;
		unsigned int logsize = get_logsize(get_slot_metadata(get_slot_id(ptr)));
		unsigned int buddy_address = ((address >> logsize) ^ 1) << logsize;
		char* buddy_ptr = (char*)buddy_address;
		if (get_logsize(get_slot_metadata(get_slot_id(buddy_ptr))) == logsize &&
				!is_used(get_slot_metadata(get_slot_id(buddy_ptr)))) {
			char* newptr;
			if (address < buddy_address) {
				newptr = (char*)address;
			} else {
				newptr = (char*)buddy_address;
			}
			list_remove((list_node_t*)ptr);
			list_remove((list_node_t*)buddy_ptr);
			bin_id++;
			size *= 2;
			list_append((list_node_t*)newptr, bin_id);
			// TODO(aanastasov): do we need to update all slot entries for this ptr?
			table_mark(newptr, size, FREE);
			ptr = newptr;
		} else {
			try_coalescing = 0;
		}
	} while (try_coalescing);
}
