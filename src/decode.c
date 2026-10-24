#include "decode.h"

/* raw'in [lo, hi] bit araligini al */
static uint32_t bits(uint32_t raw, unsigned hi, unsigned lo) {
    return (raw >> lo) & ((1u << (hi - lo + 1)) - 1);
}

/* 'width' bitlik degeri isaretli 32 bite genislet */
static int32_t sign_extend(uint32_t value, unsigned width) {
    uint32_t sign = 1u << (width - 1);
    return (int32_t)((value ^ sign) - sign);
}

char instr_format(uint32_t opcode) {
    switch (opcode) {
    case OP_REG:                                   return 'R';
    case OP_IMM: case OP_LOAD: case OP_JALR:
    case OP_SYSTEM: case OP_FENCE:                 return 'I';
    case OP_STORE:                                 return 'S';
    case OP_BRANCH:                                return 'B';
    case OP_LUI: case OP_AUIPC:                    return 'U';
    case OP_JAL:                                   return 'J';
    default:                                       return '?';
    }
}

Instr decode(uint32_t raw) {
    Instr in;
    in.raw    = raw;
    in.opcode = bits(raw, 6, 0);
    in.rd     = bits(raw, 11, 7);
    in.funct3 = bits(raw, 14, 12);
    in.rs1    = bits(raw, 19, 15);
    in.rs2    = bits(raw, 24, 20);
    in.funct7 = bits(raw, 31, 25);

    switch (instr_format(in.opcode)) {
    case 'I':   /* imm[11:0] = raw[31:20] */
        in.imm = sign_extend(bits(raw, 31, 20), 12);
        break;
    case 'S':   /* imm[11:5] = raw[31:25], imm[4:0] = raw[11:7] */
        in.imm = sign_extend((bits(raw, 31, 25) << 5) | bits(raw, 11, 7), 12);
        break;
    case 'B':   /* imm[12|10:5|4:1|11], en dusuk bit hep 0 */
        in.imm = sign_extend((bits(raw, 31, 31) << 12) |
                             (bits(raw, 7, 7)   << 11) |
                             (bits(raw, 30, 25) << 5)  |
                             (bits(raw, 11, 8)  << 1), 13);
        break;
    case 'U':   /* imm[31:12] = raw[31:12], alt 12 bit 0 */
        in.imm = (int32_t)(raw & 0xFFFFF000u);
        break;
    case 'J':   /* imm[20|10:1|11|19:12], en dusuk bit hep 0 */
        in.imm = sign_extend((bits(raw, 31, 31) << 20) |
                             (bits(raw, 19, 12) << 12) |
                             (bits(raw, 20, 20) << 11) |
                             (bits(raw, 30, 21) << 1), 21);
        break;
    default:    /* R formatinda sabit yok */
        in.imm = 0;
        break;
    }
    return in;
}
