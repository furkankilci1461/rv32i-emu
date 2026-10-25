#include <stdio.h>
#include "cpu.h"

/* Test programi: her satir bir makine komutu */
static const uint32_t program[] = {
    0x00A00093, /* addi x1,  x0, 10      -> x1  = 10          */
    0x00300113, /* addi x2,  x0, 3       -> x2  = 3           */
    0x002081B3, /* add  x3,  x1, x2      -> x3  = 13          */
    0x40208233, /* sub  x4,  x1, x2      -> x4  = 7           */
    0x00209293, /* slli x5,  x1, 2       -> x5  = 40          */
    0xFF800313, /* addi x6,  x0, -8      -> x6  = -8          */
    0x40135393, /* srai x7,  x6, 1       -> x7  = -4          */
    0x01C35413, /* srli x8,  x6, 28      -> x8  = 15          */
    0x001324B3, /* slt  x9,  x6, x1      -> x9  = 1 (-8 < 10) */
    0x00133533, /* sltu x10, x6, x1      -> x10 = 0 (isaretsiz: cok buyuk) */
    0x0FF0C593, /* xori x11, x1, 0xFF    -> x11 = 245         */
    0x00100073, /* ebreak                -> dur               */
};

/* Beklenen sonuclar: {register, deger} */
static const struct { unsigned reg; int32_t val; } expected[] = {
    {1, 10}, {2, 3}, {3, 13}, {4, 7}, {5, 40}, {6, -8},
    {7, -4}, {8, 15}, {9, 1}, {10, 0}, {11, 245},
};

int main(void) {
    CPU cpu;
    if (cpu_init(&cpu) != 0) return 1;

    /* Programi bellege 0 adresinden itibaren yukle */
    for (unsigned i = 0; i < sizeof(program) / sizeof(program[0]); i++)
        mem_write32(&cpu, i * 4, program[i]);

    int result = cpu_run(&cpu, 1000);
    cpu_dump(&cpu);

    int fails = 0;
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        int32_t got = (int32_t)reg_read(&cpu, expected[i].reg);
        if (got != expected[i].val) {
            printf("HATA: x%u = %d, beklenen %d\n", expected[i].reg, got, expected[i].val);
            fails++;
        }
    }
    if (result == STEP_HALT && fails == 0) printf("\nTUM TESTLER GECTI\n");

    cpu_free(&cpu);
    return fails ? 1 : 0;
}
