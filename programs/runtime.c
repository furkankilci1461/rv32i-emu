#include "runtime.h"

/* ---- Sistem cagrisi: write(1, buf, len) ---- */
static int sys_write(const char *buf, int len) {
    register int         a0 __asm__("a0") = 1;   /* fd = stdout */
    register const char *a1 __asm__("a1") = buf;
    register int         a2 __asm__("a2") = len;
    register int         a7 __asm__("a7") = 64;  /* SYS_WRITE */
    __asm__ volatile ("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7) : "memory");
    return a0;
}

void print_char(char c) { sys_write(&c, 1); }

void print_str(const char *s) {
    int len = 0;
    while (s[len]) len++;
    sys_write(s, len);
}

void print_int(int n) {
    char buf[12];
    int i = 11;
    unsigned u = (n < 0) ? 0u - (unsigned)n : (unsigned)n;
    buf[i] = '\0';
    do {
        buf[--i] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    if (n < 0) buf[--i] = '-';
    print_str(&buf[i]);
}

/* ---- Derleyici yardimcilari ----
 * RV32I'da carpma/bolme komutu yok (onlar "M" eklentisinde).
 * Derleyici a*b, a/b, a%b gordugunde bu fonksiyonlari cagirir. */
unsigned __mulsi3(unsigned a, unsigned b) {
    unsigned r = 0;
    while (b) {
        if (b & 1) r += a;
        a <<= 1;
        b >>= 1;
    }
    return r;
}

unsigned __udivsi3(unsigned n, unsigned d) {
    unsigned q = 0, r = 0;
    if (d == 0) return 0xFFFFFFFFu;
    for (int i = 31; i >= 0; i--) {          /* kagit-kalem bolmesi, ikilik */
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) { r -= d; q |= 1u << i; }
    }
    return q;
}

unsigned __umodsi3(unsigned n, unsigned d) {
    return n - __mulsi3(__udivsi3(n, d), d);
}

int __divsi3(int a, int b) {
    int neg = (a < 0) != (b < 0);
    unsigned q = __udivsi3(a < 0 ? 0u - (unsigned)a : (unsigned)a,
                           b < 0 ? 0u - (unsigned)b : (unsigned)b);
    return neg ? -(int)q : (int)q;
}

int __modsi3(int a, int b) {
    return a - (int)__mulsi3((unsigned)__divsi3(a, b), (unsigned)b);
}

/* Derleyici buyuk dizi kopyalama/sifirlama icin bunlari cagirabilir */
void *memset(void *dst, int c, unsigned n) {
    unsigned char *p = dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}

void *memcpy(void *dst, const void *src, unsigned n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}
