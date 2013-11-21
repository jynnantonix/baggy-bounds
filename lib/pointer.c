#include <stdlib.h>

int main(int argc, char **argv)
{
	int *p = malloc(24 * sizeof(int));
	int *q = p;
	int *a = q + 15;
	int *b = a + 10;
	int *c = b - 5;

	*c = 3;

	return 0;
}
