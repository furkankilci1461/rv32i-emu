#include "cpu.h"
#include "decode.h"
#include <stdio.h>

/* Sistem cagrilari (Linux numaralari): a7 = numara, a0..a2 = argumanlar */
enum { SYS_WRITE = 64, SYS_EXIT = 93 };

static int exec_ecall(CPU *cpu) {
    uint32_t num = reg_read(cpu, 17);      /* a7 */
    uint32_t a0  = reg_read(cpu, 10);
    uint32_t a1  = reg_read(cpu, 11);
    uint32_t a2  = reg_read(cpu, 12);

    switch (num) {
    case SYS_WRITE: {                       /* write(fd, buf, len) */
        FILE *out = (a0 == 2) ? stderr : stdout;
        for (uint32_t i = 0; i < a2; i++)
            fputc(mem_read8(cpu, a1 + i), out);
        reg_write(cpu, 10, a2);             /* yazilan bayt sayisi */
        return STEP_OK;
    }
    case SYS_EXIT:                          /* exit(code) */
        cpu->exit_code = (int32_t)a0;
        return STEP_HALT;
    default:
        fprintf(stderr, "Bilinmeyen sistem cagrisi %u (pc=0x%08x)\n", num, cpu->pc);
        return STEP_ERROR;
    }
}

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

/* Yukleme: LB, LH, LW, LBU, LHU  ->  rd = mem[rs1 + imm] */
static int exec_load(CPU *cpu, const Instr *in) {
    uint32_t addr = reg_read(cpu, in->rs1) + (uint32_t)in->imm;
    uint32_t r;

    switch (in->funct3) {
    case 0: r = (uint32_t)(int32_t)(int8_t) mem_read8 (cpu, addr); break; /* LB  (isaretli)   */
    case 1: r = (uint32_t)(int32_t)(int16_t)mem_read16(cpu, addr); break; /* LH  (isaretli)   */
    case 2: r = mem_read32(cpu, addr);                             break; /* LW               */
    case 4: r = mem_read8 (cpu, addr);                             break; /* LBU (isaretsiz)  */
    case 5: r = mem_read16(cpu, addr);                             break; /* LHU (isaretsiz)  */
    default: return -1;
    }
    reg_write(cpu, in->rd, r);
    return 0;
}

/* Saklama: SB, SH, SW  ->  mem[rs1 + imm] = rs2 */
static int exec_store(CPU *cpu, const Instr *in) {
    uint32_t addr = reg_read(cpu, in->rs1) + (uint32_t)in->imm;
    uint32_t v    = reg_read(cpu, in->rs2);

    switch (in->funct3) {
    case 0: mem_write8 (cpu, addr, (uint8_t)v);  break; /* SB */
    case 1: mem_write16(cpu, addr, (uint16_t)v); break; /* SH */
    case 2: mem_write32(cpu, addr, v);           break; /* SW */
    default: return -1;
    }
    return 0;
}

/* Dallanma: kosul dogruysa pc + imm adresine atla */
static int exec_branch(CPU *cpu, const Instr *in, uint32_t *next_pc) {
    uint32_t a = reg_read(cpu, in->rs1);
    uint32_t b = reg_read(cpu, in->rs2);
    int take;

    switch (in->funct3) {
    case 0: take = (a == b);                    break; /* BEQ  */
    case 1: take = (a != b);                    break; /* BNE  */
    case 4: take = ((int32_t)a <  (int32_t)b);  break; /* BLT  */
    case 5: take = ((int32_t)a >= (int32_t)b);  break; /* BGE  */
    case 6: take = (a <  b);                    break; /* BLTU */
    case 7: take = (a >= b);                    break; /* BGEU */
    default: return -1;
    }
    if (take) *next_pc = cpu->pc + (uint32_t)in->imm;
    return 0;
}

int cpu_step(CPU *cpu) {
    uint32_t raw = mem_read32(cpu, cpu->pc);   /* 1) FETCH  */
    Instr in = decode(raw);                    /* 2) DECODE */
    uint32_t next_pc = cpu->pc + 4;            /* varsayilan: siradaki komut */
    int err = 0;

    switch (in.opcode) {                       /* 3) EXECUTE */
    case OP_IMM:    err = exec_op_imm(cpu, &in);           break;
    case OP_REG:    err = exec_op_reg(cpu, &in);           break;
    case OP_LOAD:   err = exec_load(cpu, &in);             break;
    case OP_STORE:  err = exec_store(cpu, &in);            break;
    case OP_BRANCH: err = exec_branch(cpu, &in, &next_pc); break;

    case OP_LUI:    /* rd = imm (ust 20 bit) */
        reg_write(cpu, in.rd, (uint32_t)in.imm);
        break;
    case OP_AUIPC:  /* rd = pc + imm (ust 20 bit) */
        reg_write(cpu, in.rd, cpu->pc + (uint32_t)in.imm);
        break;
    case OP_JAL:    /* rd = donus adresi, pc = pc + imm */
        reg_write(cpu, in.rd, cpu->pc + 4);
        next_pc = cpu->pc + (uint32_t)in.imm;
        break;
    case OP_JALR: { /* hedefi ONCE hesapla: rd == rs1 olabilir */
        uint32_t target = (reg_read(cpu, in.rs1) + (uint32_t)in.imm) & ~1u;
        reg_write(cpu, in.rd, cpu->pc + 4);
        next_pc = target;
        break;
    }
    case OP_FENCE:  /* tek cekirdekli emulatorde bir sey yapmaz */
        break;
    case OP_SYSTEM:
        if (raw == 0x00100073) return STEP_HALT;   /* EBREAK */
        if (raw == 0x00000073) {                    /* ECALL */
            int s = exec_ecall(cpu);
            if (s != STEP_OK) return s;
            break;
        }
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
    if (next_pc & 3) {
        fprintf(stderr, "Hizasiz atlama hedefi 0x%08x (pc=0x%08x)\n", next_pc, cpu->pc);
        return STEP_ERROR;
    }
    cpu->pc = next_pc;                         /* 4) PC'yi ilerlet */
    cpu->instret++;
    return STEP_OK;
}

int cpu_run(CPU *cpu, uint64_t max_steps) {
    for (uint64_t i = 0; i < max_steps; i++) {
        int s = cpu_step(cpu);
        if (s != STEP_OK) return s;
    }
    fprintf(stderr, "Komut siniri asildi (%llu)\n", (unsigned long long)max_steps);
    return STEP_ERROR;
}
