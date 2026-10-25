#include "cpu.h"
#include "decode.h"
#include <stdio.h>

/* I-tipi aritmetik: ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI */
static int exec_op_imm(CPU *cpu, const Instr *in) {
    uint32_t a     = reg_read(cpu, in->rs1);
    uint32_t imm   = (uint32_t)in->imm;
    uint32_t shamt = imm & 0x1F;          /* kaydirma miktari: alt 5 bit */
    uint32_t r;

    switch (in->funct3) {
    case 0: r = a + imm;                                   break; /* ADDI  */
    case 2: r = (int32_t)a < in->imm;                      break; /* SLTI  */
    case 3: r = a < imm;                                   break; /* SLTIU */
    case 4: r = a ^ imm;                                   break; /* XORI  */
    case 6: r = a | imm;                                   break; /* ORI   */
    case 7: r = a & imm;                                   break; /* ANDI  */
    case 1: r = a << shamt;                                break; /* SLLI  */
    case 5:
        if (in->funct7 == 0x20) r = (uint32_t)((int32_t)a >> shamt); /* SRAI */
        else                    r = a >> shamt;                      /* SRLI */
        break;
    default: return -1;
    }
    reg_write(cpu, in->rd, r);
    return 0;
}

/* R-tipi aritmetik: ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND */
static int exec_op_reg(CPU *cpu, const Instr *in) {
    uint32_t a = reg_read(cpu, in->rs1);
    uint32_t b = reg_read(cpu, in->rs2);
    uint32_t shamt = b & 0x1F;
    uint32_t r;
    int alt = (in->funct7 == 0x20);       /* SUB ve SRA icin */

    switch (in->funct3) {
    case 0: r = alt ? a - b : a + b;                                  break; /* ADD/SUB */
    case 1: r = a << shamt;                                           break; /* SLL  */
    case 2: r = (int32_t)a < (int32_t)b;                              break; /* SLT  */
    case 3: r = a < b;                                                break; /* SLTU */
    case 4: r = a ^ b;                                                break; /* XOR  */
    case 5: r = alt ? (uint32_t)((int32_t)a >> shamt) : a >> shamt;   break; /* SRL/SRA */
    case 6: r = a | b;                                                break; /* OR   */
    case 7: r = a & b;                                                break; /* AND  */
    default: return -1;
    }
    reg_write(cpu, in->rd, r);
    return 0;
}

int cpu_step(CPU *cpu) {
    uint32_t raw = mem_read32(cpu, cpu->pc);   /* 1) FETCH  */
    Instr in = decode(raw);                    /* 2) DECODE */
    uint32_t next_pc = cpu->pc + 4;            /* varsayilan: siradaki komut */
    int err = 0;

    switch (in.opcode) {                       /* 3) EXECUTE */
    case OP_IMM: err = exec_op_imm(cpu, &in); break;
    case OP_REG: err = exec_op_reg(cpu, &in); break;
    case OP_SYSTEM:
        if (raw == 0x00100073) return STEP_HALT;   /* EBREAK: programi durdur */
        err = -1;
        break;
    default:
        err = -1;
        break;
    }

    if (err) {
        fprintf(stderr, "Bilinmeyen/desteklenmeyen komut 0x%08x (pc=0x%08x)\n",
                raw, cpu->pc);
        return STEP_ERROR;
    }
    cpu->pc = next_pc;                         /* 4) PC'yi ilerlet */
    return STEP_OK;
}

int cpu_run(CPU *cpu, uint64_t max_steps) {
    for (uint64_t i = 0; i < max_steps; i++) {
        int s = cpu_step(cpu);
        if (s != STEP_OK) return s;
    }
    fprintf(stderr, "Adim siniri asildi (%llu)\n", (unsigned long long)max_steps);
    return STEP_ERROR;
}
