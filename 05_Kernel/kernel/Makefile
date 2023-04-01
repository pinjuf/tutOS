all: kernel.bin kernel.elf

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=large -fomit-frame-pointer -O0 -g
LD = ld
ASM = nasm

OBJS = kentry.o main.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) -f elf64 $< -o $@

kernel.bin: $(OBJS)
	$(LD) -T linker.ld -o $@ $^

kernel.elf: $(OBJS)
	$(LD) -T linker.ld -Ttext 0xA000 --oformat elf64-x86-64 -o $@ $^

clean:
	rm -f *.o *.bin
