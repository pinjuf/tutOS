all: libtut_common.a

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables
LD = ld
ASM = nasm

OBJS = signal.o

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

%.o: %.asm
	nasm -f elf64 $< -o $@

libtut_common.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f *.o *.a
