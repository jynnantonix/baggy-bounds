#include <stdio.h>
void baggy_init();

int main() {
	baggy_init();
	printf("hi\n");
	int b = 5;
	printf("%p\n", &b);
	return 0;
}
