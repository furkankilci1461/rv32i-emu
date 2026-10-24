CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -g
SRC = src/main.c src/cpu.c

rv32i: $(SRC) src/cpu.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -rf rv32i rv32i.dSYM
