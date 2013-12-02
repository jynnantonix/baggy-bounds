#include <cstdio>

int* __attribute__ ((noinline)) inc1(int* a) {
	printf("inc1 start\n");
	return a+1;
	printf("inc1 end\n");
}

int* __attribute__ ((noinline)) dec1(int* a) {
	return a-1;
}

void __attribute__ ((noinline)) hey(int* a) {
	printf("heyo %d\n", (int)(*((int*)0x7c000000)));

	printf("hi 0\n");
	int* b = dec1(a); // a - 1
	printf("hi 1\n");
	int* c = inc1(b); // a

	printf("hi 2\n");
	int* d = inc1(a+7); // a + 8
	printf("hi 3\n");
	int* e = dec1(d); // a + 7
	printf("hi 4\n");

	printf("%p\n", a);
	printf("%p\n", b);
	printf("%p\n", c);
	printf("%p\n", d);
	printf("%p\n", e);

	int* f = inc1(d); // a + 9
	printf("%p\n", f);

	int* g = inc1(f); // a + 10
	// should segfault
	printf("%p\n", f);
}

int main() {
	int a[8];
	printf("a = %p\n", a);
	hey(a);
	return 0;
}
