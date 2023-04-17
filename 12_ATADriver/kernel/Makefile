all: kernel.bin kernel.elf

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=large -fomit-frame-pointer -O3 -g
LD = ld
ASM = nasm

OBJS = vga.o kentry.o main.o util.o gdt.o idt.o int.o isr.o pic.o paging.o mm.o kbd.o ata.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) -f elf64 $< -o $@

kernel.bin: $(OBJS)
	$(LD) -T linker.ld -o $@ $^

kernel.elf: $(OBJS)
	$(LD) -T linker.ld --oformat elf64-x86-64 -o $@ $^

clean:
	rm -f *.o *.bin *.elf
