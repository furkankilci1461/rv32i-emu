# rvasm uyumluluk testi: RV32I'nin tum komutlari + sahte komutlar.
# Bu dosya hem rvasm hem LLVM ile derlenir; iki cikti bayt bayt ayni olmali.
    .text
    .globl _start
_start:
    # R tipi
    add  x1, x2, x3
    sub  t0, t1, t2
    sll  a0, a1, a2
    slt  s0, s1, s2
    sltu x31, x30, x29
    xor  ra, sp, gp
    srl  tp, fp, s11
    sra  a7, a6, a5
    or   zero, x0, x1
    and  t6, t5, t4

    # I tipi
    addi a0, a0, -2048
    addi a0, a0, 2047
    slti t0, t1, -1
    sltiu t0, t1, 100
    xori s3, s4, 0x7ff
    ori  s5, s6, -0x800
    andi s7, s8, 255
    slli a1, a2, 0
    srli a1, a2, 31
    srai a1, a2, 7

    # Yukleme / saklama
    lb   a0, 0(sp)
    lh   a1, -4(sp)
    lw   a2, 2047(s0)
    lbu  a3, -2048(s0)
    lhu  a4, (a5)
    sb   a0, 1(sp)
    sh   a1, -2(sp)
    sw   a2, 1024(gp)

geri:
    # Dallanmalar (ileri ve geri)
    beq  a0, a1, geri
    bne  a0, a1, ileri
    blt  t0, t1, geri
    bge  t0, t1, ileri
    bltu s0, s1, geri
    bgeu s0, s1, ileri
    beqz a0, geri
    bnez a0, ileri
ileri:
    # U tipi ve atlamalar
    lui   a0, 0x12345
    lui   a0, 0xfffff
    auipc a1, 0
    auipc a1, 0x80000
    jal   ileri
    jal   x0, geri
    jal   t0, uzak
    jalr  a0
    jalr  ra, 8(a0)
    jalr  x0, -4(t0)

    # Sahte komutlar
    nop
    mv   a0, a1
    not  t0, t1
    neg  t2, t3
    li   a0, 0
    li   a0, -1
    li   a0, 2047
    li   a0, 4096
    li   a0, 0x12345678
    li   a0, 0x80000000
    li   a0, -2049
    li   a0, 0x7ffff800
    j    geri
    jr   t0
    ret
    call uzak
    la   a0, metin

    ecall
    ebreak
    fence

    .align 2
uzak:
    ret
metin:
    .word 0xdeadbeef, 42, -1
