#include <cstdio>
#include <cstdlib>

int main() {
       srand(666);

       int n = 1 << 24;
       int *a = new int[n];
       unsigned int checksum = 0;
       unsigned int counter = 0;
       for (int i = 0; i < n; ++i) {
               a[i] = rand();
       }
       for (int i = 0; i < n; ++i) {
               counter++;
               for (int j = 0; j < 3; ++j) {
                       a[n - 1 - j] -= checksum;
                       checksum += a[n - 1 - j];
               }
               int *b = &a[n - 1 - (rand() % 3)];
               ++b;
               checksum += (unsigned int)(b - a) * 12512;
       }
       printf("%u\n", checksum);
       return 0;
}
