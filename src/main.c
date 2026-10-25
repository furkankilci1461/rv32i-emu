#include <stdio.h>
#include "cpu.h"

/* Ham ikili (binary) dosyayi bellege 0 adresinden itibaren yukle */
static int load_binary(CPU *cpu, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return -1;
    }
    size_t n = fread(cpu->mem, 1, MEM_SIZE, f);
    if (!feof(f)) {
        fprintf(stderr, "%s: dosya bellege sigmiyor (en fazla %u bayt)\n", path, MEM_SIZE);
        fclose(f);
        return -1;
    }
    fclose(f);
    fprintf(stderr, "[rv32i] %s yuklendi (%zu bayt)\n", path, n);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Kullanim: %s program.bin\n", argv[0]);
        return 2;
    }

    CPU cpu;
    if (cpu_init(&cpu) != 0) {
        fprintf(stderr, "Bellek ayrilamadi\n");
        return 1;
    }
    if (load_binary(&cpu, argv[1]) != 0) {
        cpu_free(&cpu);
        return 1;
    }

    int result = cpu_run(&cpu, 1000000000ull);   /* en fazla 1 milyar komut */

    fflush(stdout);
    fprintf(stderr, "[rv32i] %llu komut calistirildi, cikis kodu %d\n",
            (unsigned long long)cpu.instret, cpu.exit_code);
    if (result == STEP_ERROR) cpu_dump(&cpu);

    int code = (result == STEP_ERROR) ? 1 : cpu.exit_code;
    cpu_free(&cpu);
    return code;
}
