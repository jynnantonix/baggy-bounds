#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "address_constants.h"

unsigned char* baggy_size_table;

unsigned int page_size;
void sigsegv_handler(int signal_number, siginfo_t* siginfo, void* context) {
	unsigned int addr = (unsigned int)siginfo->si_addr;
	if (addr >= TABLE_START && addr < TABLE_MID) {
		unsigned int addr_aligned = addr - (addr % page_size);
		mmap((void*)addr_aligned, 1, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	} else {
		printf("SIGSEGV segmentation fault\n", (void*)addr);
		exit(1);
	}
}

void buddy_allocator_init();

void setup_table() {
	baggy_size_table = (unsigned char*) TABLE_START;
	page_size = (unsigned int) sysconf(_SC_PAGE_SIZE);

	struct sigaction act;
	act.sa_sigaction = sigsegv_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &act, NULL);

	int fd = open("/dev/zero", O_RDONLY); 
	mmap((void*) TABLE_MID, TABLE_END - TABLE_MID, PROT_READ,
		MAP_PRIVATE | MAP_FIXED, fd, 0);
}

void move_stack();

void setup_stack() {
	mmap((void*) STACK_START, STACK_END - STACK_START, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

	move_stack();

	munmap((void*) 0x80000000, 1 << 30);
}

// Given the size of an allocation, return the desired aligment.
// Finds the smallest power of 2 which is at least as large as sz
// but uses the SLOT_SIZE if that is too low.
unsigned int get_alignment(unsigned int sz) {
	while ((sz & (-sz)) != sz) {
		sz += sz & (-sz);
	}
	return sz < 16 ? 16 : sz;
}
// returns the log of a power of 2
unsigned int get_lg(unsigned int sz) {
	unsigned int c = 0;
	while (sz) {
		c++;
		sz >>= 1;
	}
	return c - 1;
}
extern char** __environ;
extern char* __progname;
unsigned int fix_args_and_env(void* ptr1) {
	// When _start calls baggy_init, it passes ptr such that ptr+4 is the value of argc.
	// First, adjust it.
	unsigned int* ptr = (int*)(ptr1 + STACK_END - INITIAL_STACK_END + 4);
	int argc = *(int*)ptr;
	// Now, adjust all the argv pointers to point to the correct location.
	for (int i = 1; i <= argc; i++) {
		ptr[i] += STACK_END - INITIAL_STACK_END;
	}
	// Same for envp
	for (int i = argc + 2; ptr[i] != 0; i++) {
		ptr[i] += STACK_END - INITIAL_STACK_END;
	}
	__environ = (char**)(STACK_END - INITIAL_STACK_END + (void*) __environ);
	__progname = STACK_END - INITIAL_STACK_END + __progname;

	// return the boundary of the allocation for the arg and env information
	int alignment = get_alignment(STACK_END - (unsigned int)ptr);
	int lg = get_lg(alignment);
	for (unsigned int i = STACK_END - alignment; i < STACK_END; i += 16) {
		baggy_size_table[i/16] = lg;
	}
	return STACK_END - alignment;
}

unsigned int baggy_init(void* stack_ptr) {
	setup_table();

	setup_stack();

	int st = fix_args_and_env(stack_ptr);
		
	buddy_allocator_init();

	return st;
}
