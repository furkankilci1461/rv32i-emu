#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cpu_init(CPU *cpu) {
    memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->pc = 0;
    cpu->instret = 0;
    cpu->exit_code = 0;
    cpu->mem = calloc(MEM_SIZE, 1);
    return cpu->mem ? 0 : -1;
}

void cpu_free(CPU *cpu) {
    free(cpu->mem);
    cpu->mem = NULL;
}

uint32_t reg_read(const CPU *cpu, unsigned r) {
    return cpu->regs[r & 31];
}

void reg_write(CPU *cpu, unsigned r, uint32_t value) {
    if (r != 0)
        cpu->regs[r & 31] = value;
}

static void check_addr(uint32_t addr, uint32_t size) {
    if (addr > MEM_SIZE - size) {
        fprintf(stderr, "Bellek hatasi: 0x%08x adresine %u baytlik erisim\n",
                addr, size);
        exit(1);
    }
}

uint8_t mem_read8(const CPU *cpu, uint32_t addr) {
    check_addr(addr, 1);
    return cpu->mem[addr];
}

uint16_t mem_read16(const CPU *cpu, uint32_t addr) {
    check_addr(addr, 2);
    return (uint16_t)(cpu->mem[addr] | (cpu->mem[addr + 1] << 8));
}

uint32_t mem_read32(const CPU *cpu, uint32_t addr) {
    check_addr(addr, 4);
    return  (uint32_t)cpu->mem[addr]
         | ((uint32_t)cpu->mem[addr + 1] << 8)
         | ((uint32_t)cpu->mem[addr + 2] << 16)
         | ((uint32_t)cpu->mem[addr + 3] << 24);
}

void mem_write8(CPU *cpu, uint32_t addr, uint8_t value) {
    check_addr(addr, 1);
    cpu->mem[addr] = value;
}

void mem_write16(CPU *cpu, uint32_t addr, uint16_t value) {
    check_addr(addr, 2);
    cpu->mem[addr]     = (uint8_t)(value & 0xFF);
    cpu->mem[addr + 1] = (uint8_t)(value >> 8);
}

void mem_write32(CPU *cpu, uint32_t addr, uint32_t value) {
    check_addr(addr, 4);
    cpu->mem[addr]     = (uint8_t)(value & 0xFF);
    cpu->mem[addr + 1] = (uint8_t)((value >> 8)  & 0xFF);
    cpu->mem[addr + 2] = (uint8_t)((value >> 16) & 0xFF);
    cpu->mem[addr + 3] = (uint8_t)((value >> 24) & 0xFF);
}

void cpu_dump(const CPU *cpu) {
    printf("pc = 0x%08x\n", cpu->pc);
    for (unsigned i = 0; i < 32; i++) {
        printf("x%-2u = 0x%08x%s", i, cpu->regs[i], (i % 4 == 3) ? "\n" : "   ");
    }
}
