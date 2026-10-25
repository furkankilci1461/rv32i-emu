#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define MEM_SIZE (1024u * 1024u)  /* 1 MiB bellek */

typedef struct {
    uint32_t regs[32];  /* x0..x31 */
    uint32_t pc;        /* program counter */
    uint8_t *mem;       /* bayt dizisi olarak bellek */
} CPU;

int  cpu_init(CPU *cpu);
void cpu_free(CPU *cpu);
void cpu_dump(const CPU *cpu);

uint32_t reg_read(const CPU *cpu, unsigned r);
void     reg_write(CPU *cpu, unsigned r, uint32_t value);

uint8_t  mem_read8 (const CPU *cpu, uint32_t addr);
uint16_t mem_read16(const CPU *cpu, uint32_t addr);
uint32_t mem_read32(const CPU *cpu, uint32_t addr);
void mem_write8 (CPU *cpu, uint32_t addr, uint8_t  value);
void mem_write16(CPU *cpu, uint32_t addr, uint16_t value);
void mem_write32(CPU *cpu, uint32_t addr, uint32_t value);

/* cpu_step / cpu_run donus degerleri */
enum { STEP_OK = 0, STEP_HALT = 1, STEP_ERROR = -1 };

int cpu_step(CPU *cpu);                      /* tek komut calistir */
int cpu_run(CPU *cpu, uint64_t max_steps);   /* EBREAK'e kadar calistir */

#endif
