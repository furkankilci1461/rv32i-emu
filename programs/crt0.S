# Programin ilk calisan kodu: yigini (stack) kur, main'i cagir, exit yap
    .section .text.start
    .globl _start
_start:
    li   sp, 0x100000      # yigin bellegin en ustunden asagi buyur
    call main              # a0 = main'in donus degeri
    li   a7, 93            # SYS_EXIT
    ecall
1:  j 1b                   # buraya gelinmez
