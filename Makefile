CC = clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -g

rv32i: src/main.c
	$(CC) $(CFLAGS) -o rv32i src/main.c

clean:
	rm -f rv32i
