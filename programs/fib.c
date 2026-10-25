#include "runtime.h"

/* Ilk 25 Fibonacci sayisini yazdir */
int main(void) {
    int a = 0, b = 1;
    print_str("Fibonacci dizisi:\n");
    for (int i = 0; i < 25; i++) {
        print_int(a);
        print_char(i < 24 ? ' ' : '\n');
        int t = a + b;
        a = b;
        b = t;
    }
    return 0;
}
