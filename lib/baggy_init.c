#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "address_constants.h"

char* baggy_size_table;

unsigned int page_size;
void sigsegv_handler(int signal_number, siginfo_t* siginfo, void* context) {
	unsigned int addr = (unsigned int)siginfo->si_addr;
	if (addr >= TABLE_START && addr < TABLE_END) {
		unsigned int addr_aligned = addr - (addr % page_size);
		mmap((void*)addr_aligned, 1, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	} else {
		printf("Segmentation fault\n");
		exit(1);
	}
}

void setup_table() {
	baggy_size_table = (char*) TABLE_START;
	page_size = (unsigned int) sysconf(_SC_PAGE_SIZE);

	struct sigaction act;
	act.sa_sigaction = sigsegv_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &act, NULL);
}

void move_stack();

void setup_stack() {
	mmap((void*) STACK_START, STACK_END - STACK_START, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

	move_stack();

	munmap((void*) 0x80000000, 1 << 30);

	/*
	void* ebpval;
	asm("movl %%ebp, %0;" : "=r"(ebpval));
	printf("%p\n", ebpval);
	for (int i = 0; i < 10; i++) {
		ebpval = *((void**) ebpval);
		printf("%p\n", ebpval);
	}
	*/
}

void baggy_init() {
	static bool is_initialized = false;
	if (!is_initialized) {
		is_initialized = true;

		setup_table();

		setup_stack();
	}
}
