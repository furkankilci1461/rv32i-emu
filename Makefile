CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -g
SRC = src/main.c src/cpu.c src/decode.c src/execute.c
HDR = src/cpu.h src/decode.h

rv32i: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -rf rv32i rv32i.dSYM
