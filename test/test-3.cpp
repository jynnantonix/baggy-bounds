#include <cstdio>

int* inc1(int* a) {
	return a+1;
}

int* dec1(int* a) {
	return a-1;
}

void hey(int* a) {
	int* b = dec1(a); // a - 1
	int* c = inc1(b); // a

	int* d = inc1(a+7); // a + 8
	int* e = dec1(d); // a + 7

	printf("%p\n", a);
	printf("%p\n", b);
	printf("%p\n", c);
	printf("%p\n", d);
	printf("%p\n", e);

	int* f = inc1(d);
	printf("%p\n", f);
}

int main() {
	int a[8];
	hey(a);
	return 0;
}
