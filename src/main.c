#include <stdio.h>
#include "decode.h"

static void show(uint32_t raw, const char *asm_text) {
    Instr in = decode(raw);
    printf("0x%08x  %-20s fmt=%c opcode=0x%02x rd=%-2u f3=%u rs1=%-2u rs2=%-2u f7=0x%02x imm=%d\n",
           raw, asm_text, instr_format(in.opcode), in.opcode, in.rd,
           in.funct3, in.rs1, in.rs2, in.funct7, in.imm);
}

int main(void) {
    show(0x02A00293, "addi x5, x0, 42");    /* I: imm=42  */
    show(0xFFF00093, "addi x1, x0, -1");    /* I: imm=-1  */
    show(0x00512423, "sw x5, 8(x2)");       /* S: imm=8   */
    show(0xFE000CE3, "beq x0, x0, -8");     /* B: imm=-8  */
    show(0x12345537, "lui x10, 0x12345");   /* U: imm=0x12345000 */
    show(0x010000EF, "jal x1, 16");         /* J: imm=16  */
    show(0x00B50633, "add x12, x10, x11");  /* R: imm=0   */
    return 0;
}
