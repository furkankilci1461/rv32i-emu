#include "runtime.h"

/* Eratosthenes kalburu ile 1000'e kadar asal sayilar */
#define N 1000
static char is_composite[N + 1];

int main(void) {
    int count = 0;
    for (int i = 2; i * i <= N; i++)
        if (!is_composite[i])
            for (int j = i * i; j <= N; j += i)
                is_composite[j] = 1;

    print_str("1000'e kadar asal sayilar:\n");
    for (int i = 2; i <= N; i++) {
        if (!is_composite[i]) {
            print_int(i);
            print_char(' ');
            count++;
        }
    }
    print_str("\nToplam: ");
    print_int(count);
    print_str(" asal sayi\n");
    return 0;
}
