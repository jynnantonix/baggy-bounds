#include <cstdio>

int* __attribute__ ((noinline)) inc1(int* a) {
	return a+1;
}

int* __attribute__ ((noinline)) dec1(int* a) {
	return a-1;
}

void __attribute__ ((noinline)) hey(int* a) {
	int* b = dec1(a); // a - 1
	printf("%p\n", b);
	int* c = dec1(b); // a - 2
	printf("%p\n", c);
	int* d = dec1(c); // a - 3
	// should segfault
	printf("%p\n", d);
}

int main() {
	int a[8];
	hey(a);
	return 0;
}
