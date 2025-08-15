all:
	gcc -std=c99 \
		-D_POSIX_C_SOURCE=200809L \
		-D_XOPEN_SOURCE=700 \
		-Wall -Wextra -Werror \
		-Wno-unused-parameter \
		-fno-asm \
		-Iinclude \
		src/*.c -o shell.out

clean:
	rm -f shell.out

.PHONY: all clean