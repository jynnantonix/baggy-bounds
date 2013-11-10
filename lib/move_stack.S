.globl move_stack

// LOL the existence of this file is hilarious

// Moves the stack

#include "address_constants.h"

move_stack:
	push %ebp
	mov %esp, %ebp

	push %eax
	push %ebx

	// move the entire stack do the target area
	movl %esp, %eax
moveloop:
	movl (%eax), %ebx
	movl %ebx, STACK_END-INITIAL_STACK_END(%eax)
	addl $4, %eax
	cmpl $INITIAL_STACK_END, %eax
	jne moveloop

	// move the stack pointer
	addl $(STACK_END - INITIAL_STACK_END), %esp

	// fix the ebp and saved ebps
	addl $(STACK_END - INITIAL_STACK_END), %ebp
	movl %ebp, %eax
ebploop:
	movl (%eax), %ebx
	cmpl $0, %ebx
	je ebploop_done
	addl $(STACK_END - INITIAL_STACK_END), %ebx
	movl %ebx, (%eax)
	movl %ebx, %eax
	jmp ebploop
ebploop_done:

	pop %ebx
	pop %eax

	pop %ebp
	ret
