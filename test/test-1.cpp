#include <cstdio>

void hey(int* a) {
	printf("%p\n", a);
	printf("%p\n", a+1);
	printf("%p\n", a+2);
	printf("%p\n", a+3);
	printf("%p\n", a+4);

	printf("%d\n", a[0]);
	printf("%d\n", a[1]);
	printf("%d\n", a[2]);
	printf("%d\n", a[3]);
	printf("%d\n", a[4]);
}

int main() {
	printf("Starting...\n");
	int a[4];
	a[0] = 100;
	a[1] = 101;
	a[2] = 102;
	a[3] = 103;
	hey(a);
	return 0;
}
