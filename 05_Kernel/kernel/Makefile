all: kernel.bin

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=large -fomit-frame-pointer -O0
LD = ld
ASM = nasm

OBJS = kentry.o main.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) -f elf64 $< -o $@

kernel.bin: $(OBJS)
	$(LD) -T linker.ld -o $@ $^

clean:
	rm -f *.o *.bin
