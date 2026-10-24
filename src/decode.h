#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>

/* Cozulmus (decode edilmis) bir RV32I komutu */
typedef struct {
    uint32_t raw;     /* 32 bitlik ham komut */
    uint32_t opcode;  /* bit 6..0   : komut ailesi */
    uint32_t rd;      /* bit 11..7  : hedef register */
    uint32_t funct3;  /* bit 14..12 : alt islem */
    uint32_t rs1;     /* bit 19..15 : 1. kaynak register */
    uint32_t rs2;     /* bit 24..20 : 2. kaynak register */
    uint32_t funct7;  /* bit 31..25 : ek alt islem */
    int32_t  imm;     /* isaret genisletilmis sabit (formata gore) */
} Instr;

/* Opcode degerleri */
enum {
    OP_LUI    = 0x37,  /* U */
    OP_AUIPC  = 0x17,  /* U */
    OP_JAL    = 0x6F,  /* J */
    OP_JALR   = 0x67,  /* I */
    OP_BRANCH = 0x63,  /* B */
    OP_LOAD   = 0x03,  /* I */
    OP_STORE  = 0x23,  /* S */
    OP_IMM    = 0x13,  /* I */
    OP_REG    = 0x33,  /* R */
    OP_FENCE  = 0x0F,
    OP_SYSTEM = 0x73   /* I */
};

Instr decode(uint32_t raw);
char  instr_format(uint32_t opcode);   /* 'R','I','S','B','U','J' veya '?' */

#endif
