#include <cstdio>

void __attribute__ ((noinline)) run(char* a, char* b, char* c) {
	for (int i = 0; i < 16; i++) {
		a[i] = 1;
		c[i] = 1;
	}

	for (int i = 0; i < 64; i++) {
		b[i] = 3;
	}
}

void __attribute__ ((noinline)) output(char* a, char* b, char* c) {
	for (int i = 0; i < 16; i++) {
		printf("%d,", (int) a[i]);
	}
	printf("\n");

	for (int i = 0; i < 16; i++) {
		printf("%d,", (int) c[i]);
	}
	printf("\n");
}

int main() {
	char a[16];
	char b[33];
	char c[16];

	run(a, b, c);

	output(a, b, c);
}
