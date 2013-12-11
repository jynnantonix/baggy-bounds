#include <cstdio>
#include <cstdlib>
#include <err.h>

// This file expects an argument and a BAGGY environment variable to be passed in
// be the testing program.

void print(char* c) {
	for (int i = 0; c[i] != '\0'; i++) {
		printf("%c", c[i]);
	}
	printf("\n");
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("argc should be 2\n");
		return 0;
	}

	printf("%s\n", argv[1]);
	printf("%s\n", getenv("BAGGY"));

	print(argv[1]);
	print(getenv("BAGGY"));

	stderr = stdout;
	warnx("warning!");
}
