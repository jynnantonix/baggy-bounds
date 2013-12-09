#include <cstdio>

int* a;
int* a8;
int* am1;

unsigned int i1;
unsigned int i2;
unsigned int i3;

void __attribute__ ((noinline)) run() {
	if (a < a8) {
		printf("a < a + 8: true!\n");
	} else {
		printf("a < a + 8: false :(\n");
	}

	if (am1 < a) {
		printf("a - 1 < a: true!\n");
	} else {
		printf("a - 1 < a: false :(\n");
	}

	if (i2 - i1 == 4) {
		printf("a - (a-1) == 4: true!\n");
	} else {
		printf("a - (a-1) == 4: false :(\n");
	}

	if (i3 - i2 == 32) {
		printf("(a+8) - a) == 32: true!\n");
	} else {
		printf("(a+8) - a) == 32: false :(\n");
	}
}

void __attribute__ ((noinline)) setup_i() {
	i1 = (unsigned int) am1;
	i2 = (unsigned int) a;
	i3 = (unsigned int) a8;
}

int main() {
	int array[8];
	a = array;

	a8 = a + 8;
	am1 = a - 1;

	setup_i();

	run();

	return 0;
}
