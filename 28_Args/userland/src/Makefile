all: init test ls

CC = gcc
CFLAGS = -T linker.ld -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -L../lib -I../lib -l:libtut.a -static -Wno-unused-parameter
ASM = nasm

test: test.o
	gcc $< $(CFLAGS) -o $@

init: init.o
	gcc $< $(CFLAGS) -o $@

ls: ls.o
	gcc $< $(CFLAGS) -o $@

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

%.o: %.asm
	nasm -f elf64 $< -o $@
