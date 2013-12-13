#include <cstdio>
#include <cstdlib>

void quick_sort(int* start, int len) {
	if (len <= 1) {
		return;
	}
	int index = rand() % len;
	int pivot = start[index];
	int l = 0;
	int r = 0;
	while (r < len) {
		if (start[r] < pivot) {
			int tmp = start[r];
			start[r] = start[l];
			start[l] = tmp;
			l++;
		}
		r++;
	}
	int l2 = l;
	r = l;
	while (r < len) {
		if (start[r] == pivot) {
			int tmp = start[r];
			start[r] = start[l2];
			start[l2] = tmp;
			l2++;
		}
		r++;
	}
	if (l == l2) {
		printf("l should not equal l2");
		exit(1);
	}
	quick_sort(start, l);
	quick_sort(start+l2, len - l2);
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
		quick_sort(a, n);
		for (int i = 0; i < n; i++) {
			checksum = 41 * checksum + (unsigned int) a[i];
		}
	}

	printf("%u\n", checksum);
}
