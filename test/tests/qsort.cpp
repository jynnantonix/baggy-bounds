#include <cstdio>
#include <cstdlib>

void quick_sort(int* start, int* end) {
	if ((int)(end - start) <= 1) {
		return;
	}
	int index = rand() % ((int)(end - start));
	int pivot = start[index];
	int *l = start;
	int *r = start;
	while (r < end) {
		if (*r < pivot) {
			int tmp = *r;
			*r = *l;
			*l = tmp;
			l++;
		}
		r++;
	}
	int *l2 = l;
	r = l;
	while (r < end) {
		if (*r == pivot) {
			int tmp = *r;
			*r = *l2;
			*l2 = tmp;
			l2++;
		}
		r++;
	}
	if (l == l2) {
		printf("l should not equal l2");
		exit(1);
	}
	quick_sort(start, l);
	quick_sort(l2, end);
}

int main() {
	srand(4); // chosen by a dice roll
	          // guaranteed to be random

	int n = 1 << 24;
	int* a = new int[n];

	unsigned int checksum = 0;

	for (int num_iters = 0; num_iters <= 5; num_iters++) {
		for (int i = 0; i < n; i++) {
			a[i] = rand();
		}
		quick_sort(a, a + n);
		for (int i = 0; i < n; i++) {
			checksum = 41 * checksum + (unsigned int) a[i];
		}
	}

	printf("%u\n", checksum);
}
