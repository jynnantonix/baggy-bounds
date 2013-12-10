#include <cstdio>
#include <cstdlib>
using namespace std;

#define N 1024

void populate_matrix(int **a) {
	for (int i = 0; i < N; ++i)
		for (int j = 0; j < N; ++j) {
			a[i][j] = rand();
		}
}

void mult(int **a, int **b, int **c) {
	for (int i = 0; i < N; ++i)
		for (int j = 0; j < N; ++j) {
			c[i][j] = 0;
			for (int k = 0; k < N; ++k) {
				c[i][j] += a[i][k] * b[k][j];
			}
		}
}

int main() {
	srand(4);

	int **a, **b, **c;
	a = (int **)malloc(N * sizeof(int *));
	for (int i = 0; i < N; ++i)
		a[i] = (int *)malloc(N * sizeof(int));
	b = (int **)malloc(N * sizeof(int *));
	for (int i = 0; i < N; ++i)
		b[i] = (int *)malloc(N * sizeof(int));
	c = (int **)malloc(N * sizeof(int *));
	for (int i = 0; i < N; ++i)
		c[i] = (int *)malloc(N * sizeof(int));
	populate_matrix(a);
	populate_matrix(b);
	mult(a, b, c);
	int checksum = 0;
	for (int i = 0; i < N; ++i)
		for (int j = 0; j < N; ++j) {
			checksum = checksum * 41 + c[i][j];
		}
	printf("%d\n", checksum);

	return 0;
}
