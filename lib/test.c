#include <stdio.h>
void baggy_init();
void test_lists();

int main() {
	baggy_init();
	test_lists();
	printf("hi\n");
	int b = 5;
	printf("%p\n", &b);
	return 0;
}
