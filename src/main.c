#include <stdio.h>
#include "cpu.h"

/* Test programi: 1..100 toplami, bellek, fonksiyon cagrisi */
static const uint32_t program[] = {
    /* 0x00 */ 0x00000093, /* addi x1, x0, 0      ; toplam = 0             */
    /* 0x04 */ 0x00100113, /* addi x2, x0, 1      ; i = 1                  */
    /* 0x08 */ 0x06500193, /* addi x3, x0, 101    ; sinir                  */
    /* 0x0C */ 0x002080B3, /* loop: add x1, x1, x2 ; toplam += i           */
    /* 0x10 */ 0x00110113, /* addi x2, x2, 1      ; i++                    */
    /* 0x14 */ 0xFE311CE3, /* bne  x2, x3, loop   ; i != 101 ise basa don  */
    /* 0x18 */ 0x00010237, /* lui  x4, 0x10       ; x4 = 0x10000 (veri)    */
    /* 0x1C */ 0x00122023, /* sw   x1, 0(x4)      ; mem[x4] = toplam       */
    /* 0x20 */ 0x00022283, /* lw   x5, 0(x4)      ; x5 = mem[x4]           */
    /* 0x24 */ 0xFFF00313, /* addi x6, x0, -1     ; x6 = 0xFFFFFFFF        */
    /* 0x28 */ 0x00620223, /* sb   x6, 4(x4)      ; tek bayt: 0xFF         */
    /* 0x2C */ 0x00420383, /* lb   x7, 4(x4)      ; isaretli   -> -1       */
    /* 0x30 */ 0x00424403, /* lbu  x8, 4(x4)      ; isaretsiz  -> 255      */
    /* 0x34 */ 0x010004EF, /* jal  x9, func       ; fonksiyonu cagir       */
    /* 0x38 */ 0x00000597, /* auipc x11, 0        ; x11 = pc = 0x38        */
    /* 0x3C */ 0x00100073, /* ebreak              ; dur                    */
    /* 0x40 */ 0x00100073, /* (buraya gelinmemeli)                         */
    /* 0x44 */ 0x04D00513, /* func: addi x10, x0, 77                       */
    /* 0x48 */ 0x00048067, /* jalr x0, 0(x9)      ; geri don (return)      */
};

/* Beklenen sonuclar: {register, deger} */
static const struct { unsigned reg; int32_t val; } expected[] = {
    {1, 5050}, {2, 101}, {3, 101}, {4, 0x10000}, {5, 5050}, {6, -1},
    {7, -1}, {8, 255}, {9, 0x38}, {10, 77}, {11, 0x38},
};

int main(void) {
    CPU cpu;
    if (cpu_init(&cpu) != 0) return 1;

    for (unsigned i = 0; i < sizeof(program) / sizeof(program[0]); i++)
        mem_write32(&cpu, i * 4, program[i]);

    int result = cpu_run(&cpu, 10000);
    cpu_dump(&cpu);

    int fails = 0;
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        int32_t got = (int32_t)reg_read(&cpu, expected[i].reg);
        if (got != expected[i].val) {
            printf("HATA: x%u = %d, beklenen %d\n", expected[i].reg, got, expected[i].val);
            fails++;
        }
    }
    if (cpu.pc != 0x3C) { printf("HATA: pc = 0x%08x, beklenen 0x3c\n", cpu.pc); fails++; }
    if (result == STEP_HALT && fails == 0) printf("\nTUM TESTLER GECTI\n");

    cpu_free(&cpu);
    return fails ? 1 : 0;
}
