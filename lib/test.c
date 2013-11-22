#include <stdio.h>
void baggy_init();
void test_lists();
void test_free();
void test_malloc();
void test_realloc();

int main() {
	baggy_init();
	test_lists();
	test_free();
	test_realloc();
	test_malloc();
	return 0;
}
