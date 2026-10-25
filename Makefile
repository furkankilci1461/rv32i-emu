# ---------- Emulator ve assembler (Mac'in kendi derleyicisiyle) ----------
CC     = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -g
CORE   = src/cpu.c src/decode.c src/execute.c
HDR    = src/cpu.h src/decode.h

all: rv32i rvasm

rv32i: src/main.c $(CORE) $(HDR)
	$(CC) $(CFLAGS) -o $@ src/main.c $(CORE)

rvasm: tools/rvasm.c
	$(CC) $(CFLAGS) -o $@ tools/rvasm.c

test_cpu: tests/test_cpu.c $(CORE) $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_cpu.c $(CORE)

test: test_cpu
	./test_cpu

# ---------- RISC-V programlari ----------
LLVM_BIN   ?= $(shell brew --prefix llvm 2>/dev/null)/bin
RV_CC      = $(LLVM_BIN)/clang
RV_OBJCOPY = $(LLVM_BIN)/llvm-objcopy
RV_FLAGS   = --target=riscv32-unknown-elf -march=rv32i -mabi=ilp32 -O2 \
             -ffreestanding -nostdlib -mno-relax -fuse-ld=lld \
             -Wl,-T,programs/link.ld
RT         = programs/crt0.S programs/runtime.c

# C programlari: LLVM ile derlenir
programs/%.bin: programs/%.c $(RT) programs/runtime.h programs/link.ld
	$(RV_CC) $(RV_FLAGS) -o programs/$*.elf $(RT) $<
	$(RV_OBJCOPY) -O binary programs/$*.elf $@

# Assembly programlari: KENDI assembler'imizla (rvasm) derlenir
programs/%.bin: programs/%.s rvasm
	./rvasm $< $@

progs: programs/fib.bin programs/primes.bin programs/hello.bin

run: rv32i progs
	./rv32i programs/hello.bin
	./rv32i programs/fib.bin
	./rv32i programs/primes.bin

# rvasm'in ciktisini LLVM'in ciktisiyla bayt bayt karsilastir
asm-check: rvasm
	./rvasm tests/asm/all_instr.s tests/asm/all_instr.rvasm.bin
	$(RV_CC) $(RV_FLAGS) -o tests/asm/all_instr.elf tests/asm/all_instr.s
	$(RV_OBJCOPY) -O binary tests/asm/all_instr.elf tests/asm/all_instr.llvm.bin
	cmp tests/asm/all_instr.rvasm.bin tests/asm/all_instr.llvm.bin
	@echo "rvasm ciktisi LLVM ile bayt bayt ayni"

clean:
	rm -rf rv32i rvasm test_cpu *.dSYM programs/*.elf programs/*.bin tests/asm/*.bin tests/asm/*.elf

.PHONY: all test progs run asm-check clean
