# ---------- Emulator (Mac'in kendi derleyicisiyle) ----------
CC     = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -g
CORE   = src/cpu.c src/decode.c src/execute.c
HDR    = src/cpu.h src/decode.h

rv32i: src/main.c $(CORE) $(HDR)
	$(CC) $(CFLAGS) -o $@ src/main.c $(CORE)

test_cpu: tests/test_cpu.c $(CORE) $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_cpu.c $(CORE)

test: test_cpu
	./test_cpu

# ---------- RISC-V programlari (LLVM capraz derleyicisiyle) ----------
LLVM_BIN   ?= $(shell brew --prefix llvm 2>/dev/null)/bin
RV_CC      = $(LLVM_BIN)/clang
RV_OBJCOPY = $(LLVM_BIN)/llvm-objcopy
RV_FLAGS   = --target=riscv32-unknown-elf -march=rv32i -mabi=ilp32 -O2 \
             -ffreestanding -nostdlib -mno-relax -fuse-ld=lld \
             -Wl,-T,programs/link.ld
RT         = programs/crt0.S programs/runtime.c

programs/%.bin: programs/%.c $(RT) programs/runtime.h programs/link.ld
	$(RV_CC) $(RV_FLAGS) -o programs/$*.elf $(RT) $<
	$(RV_OBJCOPY) -O binary programs/$*.elf $@

progs: programs/fib.bin programs/primes.bin

run: rv32i progs
	./rv32i programs/fib.bin
	./rv32i programs/primes.bin

clean:
	rm -rf rv32i test_cpu *.dSYM programs/*.elf programs/*.bin

.PHONY: test progs run clean
