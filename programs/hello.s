# hello.s - rvasm ile derlenip rv32i emulatorunde calisan program
# 1'den 100'e kadar toplar, sonucu bir fonksiyonla kontrol eder,
# sonucu ekrana yazar ve cikar.

    .text
_start:
    li   sp, 0x100000        # yigini kur

    la   a0, selam
    call yaz                 # "Merhaba ..." yaz

    # toplam = 1 + 2 + ... + 100
    li   t0, 0               # toplam
    li   t1, 1               # i
    li   t2, 101             # sinir
dongu:
    add  t0, t0, t1
    addi t1, t1, 1
    bne  t1, t2, dongu

    li   t3, 5050
    la   a0, dogru
    beq  t0, t3, sonuc
    la   a0, yanlis
sonuc:
    call yaz

    li   a0, 0               # cikis kodu 0
    li   a7, 93              # SYS_EXIT
    ecall

# yaz(a0 = sifirla biten metin): metni stdout'a yazar
yaz:
    mv   a1, a0              # a1 = metnin basi
    mv   a2, a0
uzunluk:
    lbu  t0, 0(a2)           # sifir bayta kadar say
    beqz t0, yazdir
    addi a2, a2, 1
    j    uzunluk
yazdir:
    sub  a2, a2, a1          # a2 = uzunluk
    li   a0, 1               # fd = stdout
    li   a7, 64              # SYS_WRITE
    ecall
    ret

selam:  .asciz "Merhaba! Bu program benim assembler'imla derlendi.\n"
dogru:  .asciz "1 + 2 + ... + 100 = 5050 (dogru)\n"
yanlis: .asciz "Toplam yanlis!\n"
