#include <cstdio>
extern "C" {
	void* buddy_malloc (size_t size);
}

void hey(int* a) {
	printf("%p\n", a);
	printf("%p\n", a+1);
	printf("%p\n", a+2);
	printf("%p\n", a+3);
	printf("%p\n", a+4);
	printf("%p\n", a+5);
	printf("%p\n", a+6);
	printf("%p\n", a+7);
	printf("%p\n", a+8);

	printf("%d\n", a[0]);
	printf("%d\n", a[1]);
	printf("%d\n", a[2]);
	printf("%d\n", a[3]);
	printf("%d\n", a[4]);
	printf("%d\n", a[5]);
	printf("%d\n", a[6]);
	printf("%d\n", a[7]);
	printf("%d\n", a[8]);
}

int main() {
	int* a = (int*)buddy_malloc(20);
	a[0] = 100;
	a[1] = 101;
	a[2] = 102;
	a[3] = 103;
	a[4] = 104;
	hey(a);
	return 0;
}
