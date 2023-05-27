all: libtut.a

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables
LD = ld
ASM = nasm

OBJS = libtut.o syscall.o unistd.o

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

%.o: %.asm
	nasm -f elf64 $< -o $@

libtut.a: $(OBJS)
	ar rcs $@ $^

clean:
	rm -f *.o *.a
