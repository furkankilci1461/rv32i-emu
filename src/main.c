#include <stdio.h>
#include "cpu.h"

int main(void) {
    CPU cpu;
    if (cpu_init(&cpu) != 0) {
        fprintf(stderr, "Bellek ayrilamadi\n");
        return 1;
    }

    mem_write32(&cpu, 0x100, 0x12345678);
    printf("0x100'deki 32-bit deger: 0x%08x\n", mem_read32(&cpu, 0x100));
    printf("0x100'deki ilk bayt:     0x%02x (little-endian: 78 olmali)\n",
           mem_read8(&cpu, 0x100));

    reg_write(&cpu, 5, 42);
    reg_write(&cpu, 0, 99);
    cpu_dump(&cpu);

    cpu_free(&cpu);
    return 0;
}
