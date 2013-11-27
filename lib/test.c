#include <stdio.h>
#include <assert.h>
#include "list.h"

#define NUM_BINS 32

extern unsigned int heap_size;

void baggy_init();
void* buddy_malloc(size_t);
void* buddy_realloc(void*, size_t);
void buddy_free(void*);

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
	buddy_free(arr);
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
	buddy_free(arr);

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
	for (i = 0; i < 128; ++i) {
		buddy_free(block[i]);
	}

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
	for (i = 0; i < 24; ++i) {
		buddy_free(segment[i]);
	}
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
	buddy_free(arr);
	printf("simple buddy_free test passed..\n");
	fflush(stdout);
}

void test_coalesce() {
	int* a[1 << 6];
	int i, j;
	for (i = 0; i < (1 << 6); ++i) {
		a[i] = (int*)buddy_malloc(sizeof(int) * (1 << 15));
		for (j = 0; j < (1 << 15); ++j) {
			a[i][j] = j * j + j * 12626 - 2212;
		}
	}
	unsigned int bin_id;
	unsigned int free_above_before = 0;
	for (bin_id = 17 + 1; bin_id <= 32; ++bin_id) {
		list_node_t* at = dummy_first[bin_id];
		while (at->next != dummy_last[bin_id]) {
			free_above_before += (unsigned int)(1 << bin_id);
			at = at->next;
		}
	}
	for (i = 1; i < (1 << 6); ++i)
		buddy_free(a[i]);
	for (j = 0; j < (1 << 15); ++j) {
		assert(a[0][j] == j * j + j * 12626 - 2212);
	}
	buddy_free(a[0]);
	unsigned int free_above_after = 0;
	for (bin_id = 17 + 2; bin_id <= 32; ++bin_id) {
		list_node_t* at = dummy_first[bin_id];
		while (at->next != dummy_last[bin_id]) {
			free_above_after += (unsigned int)(1 << bin_id);
			at = at->next;
		}
	}
	printf("before assert!\n");
	assert(free_above_before < free_above_after);
	printf("coalesce test passed..\n");
	fflush(stdout);
}

int main() {
	baggy_init();
	test_coalesce();
	test_lists();
	test_free();
	test_realloc();
	test_malloc();
	printf("hi\n");
	return 0;
}
