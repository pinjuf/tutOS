all: test

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -L../lib -I../lib -l:libtut.a -static
ASM = nasm

test: test.o
	gcc $< $(CFLAGS) -o $@

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

%.o: %.asm
	nasm -f elf64 $< -o $@
