#include <stdio.h>
void baggy_init();
void test_lists();
void test_free();
void test_malloc();
void test_realloc();
void test_coalesce();

int main() {
	baggy_init();
	test_coalesce();
	test_lists();
	test_free();
	test_realloc();
	test_malloc();
	printf("hi\n");
	return 0;
}
