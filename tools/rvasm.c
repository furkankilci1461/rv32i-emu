/*
 * rvasm - RV32I icin mini assembler
 *
 * Kullanim: rvasm [-l] girdi.s cikti.bin
 *   -l : her komutun adresini ve makine kodunu listele
 *
 * Iki gecisli (two-pass) calisir:
 *   1. gecis: her satirin kac bayt tuttugunu hesaplar, etiketlerin adreslerini bulur
 *   2. gecis: etiket adresleri artik bilindigi icin makine kodunu uretir
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LABELS  1024
#define MAX_OUT     (1024 * 1024)
#define MAX_OPS     4

/* ------------------------------------------------------------------ */
/* Global durum                                                        */
/* ------------------------------------------------------------------ */
typedef struct { char name[64]; uint32_t addr; } Label;

static Label    labels[MAX_LABELS];
static int      nlabels;
static uint8_t  out[MAX_OUT];
static uint32_t pc;          /* su anki adres */
static int      pass;        /* 1 veya 2 */
static int      lineno;
static int      listing;     /* -l verildi mi */
static const char *cur_src;  /* listeleme icin satirin kendisi */

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "satir %d: hata: ", lineno);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

/* ------------------------------------------------------------------ */
/* Etiketler                                                           */
/* ------------------------------------------------------------------ */
static void define_label(const char *name) {
    if (pass != 1) return;
    for (int i = 0; i < nlabels; i++)
        if (strcmp(labels[i].name, name) == 0) die("'%s' etiketi iki kez tanimlanmis", name);
    if (nlabels == MAX_LABELS) die("cok fazla etiket");
    if (strlen(name) >= sizeof(labels[0].name)) die("etiket adi cok uzun");
    strcpy(labels[nlabels].name, name);
    labels[nlabels].addr = pc;
    nlabels++;
}

static int find_label(const char *name, uint32_t *addr) {
    for (int i = 0; i < nlabels; i++)
        if (strcmp(labels[i].name, name) == 0) { *addr = labels[i].addr; return 1; }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Cikti                                                               */
/* ------------------------------------------------------------------ */
static void emit8(uint8_t b) {
    if (pc >= MAX_OUT) die("program cok buyuk");
    if (pass == 2) out[pc] = b;
    pc++;
}

static void emit32(uint32_t w) {
    if (pass == 2 && listing)
        printf("%08x:  %08x    %s\n", pc, w, cur_src);
    for (int i = 0; i < 4; i++) emit8((uint8_t)(w >> (8 * i)));   /* little-endian */
}

/* ------------------------------------------------------------------ */
/* Operand ayristirma                                                  */
/* ------------------------------------------------------------------ */
static int parse_reg(const char *s) {
    static const char *abi[32] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8",   "s9", "s10","s11","t3", "t4", "t5", "t6"
    };
    if (s[0] == 'x' && isdigit((unsigned char)s[1])) {
        char *end;
        long n = strtol(s + 1, &end, 10);
        if (*end == '\0' && n >= 0 && n < 32) return (int)n;
    }
    if (strcmp(s, "fp") == 0) return 8;
    for (int i = 0; i < 32; i++)
        if (strcmp(s, abi[i]) == 0) return i;
    die("gecersiz register '%s'", s);
    return -1;
}

static int is_number(const char *s) {
    if (*s == '-' || *s == '+') s++;
    return isdigit((unsigned char)*s);
}

static int32_t parse_imm(const char *s) {
    char *end;
    if (!is_number(s)) die("sayi bekleniyordu, '%s' bulundu", s);
    long long v = strtoll(s, &end, 0);       /* 0x.. ve negatif sayilari da anlar */
    if (*end != '\0') die("gecersiz sayi '%s'", s);
    if (v < -2147483648LL || v > 4294967295LL) die("sayi 32 bite sigmiyor: %s", s);
    return (int32_t)(uint32_t)v;
}

/* Etiket ya da sayi: etiketse adresini, sayiysa pc'ye gore hedefi dondurur */
static uint32_t parse_target(const char *s) {
    if (is_number(s)) return pc + (uint32_t)parse_imm(s);   /* sayisal ofset */
    uint32_t addr;
    if (find_label(s, &addr)) return addr;
    if (pass == 2) die("tanimsiz etiket '%s'", s);
    return pc;   /* 1. gecis: henuz bilinmiyor, boyut zaten belli */
}

/* "imm(reg)" bicimi: lw a0, 8(sp) */
static void parse_mem(const char *s, int32_t *imm, int *reg) {
    const char *open = strchr(s, '(');
    const char *close = strrchr(s, ')');
    if (!open || !close || close < open || close[1] != '\0')
        die("bellek operandi 'sayi(register)' biciminde olmali: '%s'", s);
    char num[64] = "0", r[16];
    size_t nlen = (size_t)(open - s), rlen = (size_t)(close - open - 1);
    if (nlen >= sizeof(num) || rlen >= sizeof(r)) die("operand cok uzun: '%s'", s);
    if (nlen > 0) { memcpy(num, s, nlen); num[nlen] = '\0'; }
    memcpy(r, open + 1, rlen); r[rlen] = '\0';
    *imm = parse_imm(num);
    *reg = parse_reg(r);
}

static void check_range(int32_t v, int32_t lo, int32_t hi, const char *what) {
    if (v < lo || v > hi) die("%s aralik disinda: %d (%d..%d olmali)", what, v, lo, hi);
}

/* ------------------------------------------------------------------ */
/* Kodlayicilar: alanlari 32 bitlik komuta yerlestirir                 */
/* (Adim 3'teki decode'un tam tersi)                                    */
/* ------------------------------------------------------------------ */
static uint32_t enc_r(uint32_t f7, int rs2, int rs1, uint32_t f3, int rd, uint32_t op) {
    return (f7 << 25) | ((uint32_t)rs2 << 20) | ((uint32_t)rs1 << 15) |
           (f3 << 12) | ((uint32_t)rd << 7) | op;
}
static uint32_t enc_i(int32_t imm, int rs1, uint32_t f3, int rd, uint32_t op) {
    check_range(imm, -2048, 2047, "12 bitlik sabit");
    return (((uint32_t)imm & 0xFFF) << 20) | ((uint32_t)rs1 << 15) |
           (f3 << 12) | ((uint32_t)rd << 7) | op;
}
static uint32_t enc_s(int32_t imm, int rs2, int rs1, uint32_t f3, uint32_t op) {
    check_range(imm, -2048, 2047, "12 bitlik sabit");
    uint32_t u = (uint32_t)imm;
    return (((u >> 5) & 0x7F) << 25) | ((uint32_t)rs2 << 20) | ((uint32_t)rs1 << 15) |
           (f3 << 12) | ((u & 0x1F) << 7) | op;
}
static uint32_t enc_b(int32_t off, int rs2, int rs1, uint32_t f3, uint32_t op) {
    check_range(off, -4096, 4094, "dallanma mesafesi");
    if (off & 1) die("dallanma mesafesi cift sayi olmali");
    uint32_t u = (uint32_t)off;
    return (((u >> 12) & 1) << 31) | (((u >> 5) & 0x3F) << 25) |
           ((uint32_t)rs2 << 20) | ((uint32_t)rs1 << 15) | (f3 << 12) |
           (((u >> 1) & 0xF) << 8) | (((u >> 11) & 1) << 7) | op;
}
static uint32_t enc_u(int32_t imm20, int rd, uint32_t op) {
    if (imm20 < 0 || imm20 > 0xFFFFF) die("20 bitlik sabit aralik disinda: %d", imm20);
    return ((uint32_t)imm20 << 12) | ((uint32_t)rd << 7) | op;
}
static uint32_t enc_j(int32_t off, int rd, uint32_t op) {
    check_range(off, -(1 << 20), (1 << 20) - 2, "atlama mesafesi");
    if (off & 1) die("atlama mesafesi cift sayi olmali");
    uint32_t u = (uint32_t)off;
    return (((u >> 20) & 1) << 31) | (((u >> 1) & 0x3FF) << 21) |
           (((u >> 11) & 1) << 20) | (((u >> 12) & 0xFF) << 12) |
           ((uint32_t)rd << 7) | op;
}

/* 32 bitlik bir degeri ust 20 + alt 12 bite bol (addi isaret genislettigi icin +0x800) */
static void split_hi_lo(int32_t v, int32_t *hi, int32_t *lo) {
    *hi = (int32_t)((((uint32_t)v + 0x800u) >> 12) & 0xFFFFF);
    *lo = (int32_t)((uint32_t)v - ((uint32_t)*hi << 12));
}

/* ------------------------------------------------------------------ */
/* Komut tablosu                                                       */
/* ------------------------------------------------------------------ */
enum Kind { K_R, K_I, K_SHIFT, K_LOAD, K_STORE, K_BRANCH, K_U, K_JAL, K_JALR, K_SYS };

typedef struct {
    const char *name;
    enum Kind kind;
    uint32_t opcode, f3, f7;
} Op;

static const Op ops[] = {
    {"add",  K_R, 0x33, 0, 0x00}, {"sub",  K_R, 0x33, 0, 0x20},
    {"sll",  K_R, 0x33, 1, 0x00}, {"slt",  K_R, 0x33, 2, 0x00},
    {"sltu", K_R, 0x33, 3, 0x00}, {"xor",  K_R, 0x33, 4, 0x00},
    {"srl",  K_R, 0x33, 5, 0x00}, {"sra",  K_R, 0x33, 5, 0x20},
    {"or",   K_R, 0x33, 6, 0x00}, {"and",  K_R, 0x33, 7, 0x00},

    {"addi", K_I, 0x13, 0, 0}, {"slti", K_I, 0x13, 2, 0}, {"sltiu", K_I, 0x13, 3, 0},
    {"xori", K_I, 0x13, 4, 0}, {"ori",  K_I, 0x13, 6, 0}, {"andi",  K_I, 0x13, 7, 0},

    {"slli", K_SHIFT, 0x13, 1, 0x00}, {"srli", K_SHIFT, 0x13, 5, 0x00},
    {"srai", K_SHIFT, 0x13, 5, 0x20},

    {"lb", K_LOAD, 0x03, 0, 0}, {"lh",  K_LOAD, 0x03, 1, 0}, {"lw", K_LOAD, 0x03, 2, 0},
    {"lbu", K_LOAD, 0x03, 4, 0}, {"lhu", K_LOAD, 0x03, 5, 0},

    {"sb", K_STORE, 0x23, 0, 0}, {"sh", K_STORE, 0x23, 1, 0}, {"sw", K_STORE, 0x23, 2, 0},

    {"beq", K_BRANCH, 0x63, 0, 0}, {"bne",  K_BRANCH, 0x63, 1, 0},
    {"blt", K_BRANCH, 0x63, 4, 0}, {"bge",  K_BRANCH, 0x63, 5, 0},
    {"bltu", K_BRANCH, 0x63, 6, 0}, {"bgeu", K_BRANCH, 0x63, 7, 0},

    {"lui", K_U, 0x37, 0, 0}, {"auipc", K_U, 0x17, 0, 0},
    {"jal", K_JAL, 0x6F, 0, 0}, {"jalr", K_JALR, 0x67, 0, 0},

    {"ecall", K_SYS, 0x00000073, 0, 0}, {"ebreak", K_SYS, 0x00100073, 0, 0},
    {"fence", K_SYS, 0x0FF0000F, 0, 0},
};

static void need(int n, int want, const char *mn) {
    if (n != want) die("'%s' %d operand bekler, %d verildi", mn, want, n);
}

/* Gercek (donanimda olan) bir komutu kodla */
static int assemble_real(const char *mn, char **a, int n) {
    const Op *op = NULL;
    for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); i++)
        if (strcmp(ops[i].name, mn) == 0) { op = &ops[i]; break; }
    if (!op) return 0;

    int32_t imm;
    int r;
    switch (op->kind) {
    case K_R:
        need(n, 3, mn);
        emit32(enc_r(op->f7, parse_reg(a[2]), parse_reg(a[1]), op->f3, parse_reg(a[0]), op->opcode));
        break;
    case K_I:
        need(n, 3, mn);
        emit32(enc_i(parse_imm(a[2]), parse_reg(a[1]), op->f3, parse_reg(a[0]), op->opcode));
        break;
    case K_SHIFT:
        need(n, 3, mn);
        imm = parse_imm(a[2]);
        check_range(imm, 0, 31, "kaydirma miktari");
        emit32(enc_r(op->f7, imm, parse_reg(a[1]), op->f3, parse_reg(a[0]), op->opcode));
        break;
    case K_LOAD:      /* lw rd, imm(rs1) */
        need(n, 2, mn);
        parse_mem(a[1], &imm, &r);
        emit32(enc_i(imm, r, op->f3, parse_reg(a[0]), op->opcode));
        break;
    case K_STORE:     /* sw rs2, imm(rs1) */
        need(n, 2, mn);
        parse_mem(a[1], &imm, &r);
        emit32(enc_s(imm, parse_reg(a[0]), r, op->f3, op->opcode));
        break;
    case K_BRANCH: {  /* beq rs1, rs2, etiket */
        need(n, 3, mn);
        int32_t off = (int32_t)(parse_target(a[2]) - pc);
        emit32(enc_b(off, parse_reg(a[1]), parse_reg(a[0]), op->f3, op->opcode));
        break;
    }
    case K_U:
        need(n, 2, mn);
        emit32(enc_u(parse_imm(a[1]), parse_reg(a[0]), op->opcode));
        break;
    case K_JAL: {     /* jal etiket  |  jal rd, etiket */
        if (n != 1 && n != 2) die("'jal' 1 ya da 2 operand bekler");
        int rd = (n == 2) ? parse_reg(a[0]) : 1;
        int32_t off = (int32_t)(parse_target(a[n - 1]) - pc);
        emit32(enc_j(off, rd, op->opcode));
        break;
    }
    case K_JALR:      /* jalr rs1  |  jalr rd, imm(rs1)  |  jalr rd, rs1, imm */
        if (n == 1) {
            emit32(enc_i(0, parse_reg(a[0]), 0, 1, op->opcode));
        } else if (n == 2) {
            parse_mem(a[1], &imm, &r);
            emit32(enc_i(imm, r, 0, parse_reg(a[0]), op->opcode));
        } else {
            need(n, 3, mn);
            emit32(enc_i(parse_imm(a[2]), parse_reg(a[1]), 0, parse_reg(a[0]), op->opcode));
        }
        break;
    case K_SYS:
        need(n, 0, mn);
        emit32(op->opcode);
        break;
    }
    return 1;
}

/* li: deger 12 bite sigiyorsa 1, sigmiyorsa 2 komut */
static void emit_li(int rd, int32_t v) {
    if (v >= -2048 && v <= 2047) {
        emit32(enc_i(v, 0, 0, rd, 0x13));                 /* addi rd, zero, v */
        return;
    }
    int32_t hi, lo;
    split_hi_lo(v, &hi, &lo);
    emit32(enc_u(hi, rd, 0x37));                           /* lui  rd, hi      */
    if (lo != 0) emit32(enc_i(lo, rd, 0, rd, 0x13));       /* addi rd, rd, lo  */
}

/* Sahte (pseudo) komutlar: aslinda baska komutlarin kisaltmasi */
static int assemble_pseudo(const char *mn, char **a, int n) {
    if (!strcmp(mn, "nop"))  { need(n, 0, mn); emit32(enc_i(0, 0, 0, 0, 0x13)); }
    else if (!strcmp(mn, "mv"))   { need(n, 2, mn); emit32(enc_i(0, parse_reg(a[1]), 0, parse_reg(a[0]), 0x13)); }
    else if (!strcmp(mn, "not"))  { need(n, 2, mn); emit32(enc_i(-1, parse_reg(a[1]), 4, parse_reg(a[0]), 0x13)); }
    else if (!strcmp(mn, "neg"))  { need(n, 2, mn); emit32(enc_r(0x20, parse_reg(a[1]), 0, 0, parse_reg(a[0]), 0x33)); }
    else if (!strcmp(mn, "li"))   { need(n, 2, mn); emit_li(parse_reg(a[0]), parse_imm(a[1])); }
    else if (!strcmp(mn, "j"))    { need(n, 1, mn); emit32(enc_j((int32_t)(parse_target(a[0]) - pc), 0, 0x6F)); }
    else if (!strcmp(mn, "jr"))   { need(n, 1, mn); emit32(enc_i(0, parse_reg(a[0]), 0, 0, 0x67)); }
    else if (!strcmp(mn, "ret"))  { need(n, 0, mn); emit32(enc_i(0, 1, 0, 0, 0x67)); }
    else if (!strcmp(mn, "beqz")) { need(n, 2, mn); emit32(enc_b((int32_t)(parse_target(a[1]) - pc), 0, parse_reg(a[0]), 0, 0x63)); }
    else if (!strcmp(mn, "bnez")) { need(n, 2, mn); emit32(enc_b((int32_t)(parse_target(a[1]) - pc), 0, parse_reg(a[0]), 1, 0x63)); }
    else if (!strcmp(mn, "la") || !strcmp(mn, "call")) {
        /* auipc + addi/jalr: pc'ye gore 32 bitlik adres */
        int is_call = !strcmp(mn, "call");
        need(n, is_call ? 1 : 2, mn);
        int rd = is_call ? 1 : parse_reg(a[0]);
        int32_t hi, lo;
        split_hi_lo((int32_t)(parse_target(a[n - 1]) - pc), &hi, &lo);
        emit32(enc_u(hi, rd, 0x17));                                   /* auipc rd, hi   */
        if (is_call) emit32(enc_i(lo, rd, 0, rd, 0x67));               /* jalr ra, lo(ra) */
        else         emit32(enc_i(lo, rd, 0, rd, 0x13));               /* addi rd, rd, lo */
    }
    else return 0;
    return 1;
}

/* ------------------------------------------------------------------ */
/* Direktifler: .word, .byte, .asciz, .align (digerleri yok sayilir)   */
/* ------------------------------------------------------------------ */
static void emit_string(const char *s) {
    if (*s != '"') die("tirnak icinde metin bekleniyordu");
    for (s++; *s && *s != '"'; s++) {
        char c = *s;
        if (c == '\\') {
            s++;
            switch (*s) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case '0': c = '\0'; break;
            case '\\': c = '\\'; break;
            case '"': c = '"'; break;
            default: die("bilinmeyen kacis dizisi '\\%c'", *s);
            }
        }
        emit8((uint8_t)c);
    }
    if (*s != '"') die("kapanmamis tirnak");
    emit8(0);
}

static int assemble_directive(const char *mn, char **a, int n, const char *rest) {
    if (mn[0] != '.') return 0;
    if (!strcmp(mn, ".word")) {
        for (int i = 0; i < n; i++) emit32((uint32_t)parse_imm(a[i]));
    } else if (!strcmp(mn, ".byte")) {
        for (int i = 0; i < n; i++) emit8((uint8_t)parse_imm(a[i]));
    } else if (!strcmp(mn, ".asciz") || !strcmp(mn, ".string")) {
        emit_string(rest);
    } else if (!strcmp(mn, ".align")) {      /* .align 2 -> 4 bayta hizala */
        need(n, 1, mn);
        uint32_t al = 1u << parse_imm(a[0]);
        while (pc % al) emit8(0);
    }
    /* .text, .globl, .section vb. : bu basit assembler icin anlamsiz, yok say */
    return 1;
}

/* ------------------------------------------------------------------ */
/* Satir isleme                                                        */
/* ------------------------------------------------------------------ */
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}

static void strip_comment(char *s) {
    int in_str = 0;
    char prev = '\0';
    for (; *s; prev = *s, s++) {
        if (*s == '"' && prev != '\\') in_str = !in_str;
        if (!in_str && (*s == '#' || (*s == '/' && s[1] == '/'))) { *s = '\0'; return; }
    }
}

static void process_line(char *line) {
    char src_copy[256];
    snprintf(src_copy, sizeof(src_copy), "%s", trim(line));
    cur_src = src_copy;

    strip_comment(line);
    char *s = trim(line);

    /* Etiketler: "dongu:" (ayni satirda komut da olabilir) */
    for (;;) {
        char *colon = strchr(s, ':');
        if (!colon) break;
        char *p = s;
        while (p < colon && (isalnum((unsigned char)*p) || *p == '_' || *p == '.')) p++;
        if (p != colon || p == s) break;
        *colon = '\0';
        define_label(s);
        s = trim(colon + 1);
    }
    if (*s == '\0') return;

    /* Komut adi */
    char *mn = s;
    while (*s && !isspace((unsigned char)*s)) s++;
    if (*s) *s++ = '\0';
    for (char *p = mn; *p; p++) *p = (char)tolower((unsigned char)*p);
    char *rest = trim(s);

    /* Operandlar: virgulle ayrilmis */
    char *args[MAX_OPS + 1];
    int n = 0;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", rest);
    if (buf[0] && strcmp(mn, ".asciz") && strcmp(mn, ".string")) {
        for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) {
            if (n == MAX_OPS) die("cok fazla operand");
            args[n] = trim(tok);
            for (char *p = args[n]; *p; p++) *p = (char)tolower((unsigned char)*p);
            n++;
        }
    }

    if (assemble_directive(mn, args, n, rest)) return;
    if (assemble_real(mn, args, n)) return;
    if (assemble_pseudo(mn, args, n)) return;
    die("bilinmeyen komut '%s'", mn);
}

int main(int argc, char **argv) {
    int ai = 1;
    if (ai < argc && strcmp(argv[ai], "-l") == 0) { listing = 1; ai++; }
    if (argc - ai != 2) {
        fprintf(stderr, "Kullanim: %s [-l] girdi.s cikti.bin\n", argv[0]);
        return 2;
    }

    FILE *f = fopen(argv[ai], "r");
    if (!f) { perror(argv[ai]); return 1; }

    char line[256];
    for (pass = 1; pass <= 2; pass++) {
        rewind(f);
        pc = 0;
        lineno = 0;
        while (fgets(line, sizeof(line), f)) {
            lineno++;
            line[strcspn(line, "\r\n")] = '\0';
            process_line(line);
        }
    }
    fclose(f);

    FILE *o = fopen(argv[ai + 1], "wb");
    if (!o) { perror(argv[ai + 1]); return 1; }
    fwrite(out, 1, pc, o);
    fclose(o);
    fprintf(stderr, "[rvasm] %s -> %s (%u bayt, %d etiket)\n", argv[ai], argv[ai + 1], pc, nlabels);
    return 0;
}
