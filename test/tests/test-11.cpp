#include <cstdio>
#include <cstdlib>

int* __attribute__ ((noinline)) inc1(int* a) {
	return a+1;
}

int* __attribute__ ((noinline)) dec1(int* a) {
	return a-1;
}

void __attribute__ ((noinline)) hey(int* a) {
	int* b = dec1(a); // a - 1
	int* c = inc1(b); // a

	int* d = inc1(a+7); // a + 8
	int* e = dec1(d); // a + 7

	printf("%p\n", a);
	printf("%p\n", b);
	printf("%p\n", c);
	printf("%p\n", d);
	printf("%p\n", e);

	int* f = inc1(d); // a + 9
	printf("%p\n", f);

	int* g = inc1(f); // a + 10
	// should segfault
	printf("%p\n", g);
}

int main() {
	int* a = (int *)malloc(32);
	hey(a);
	return 0;
}
