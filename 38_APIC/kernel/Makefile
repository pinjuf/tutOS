all: kernel.bin kernel.elf

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mcmodel=large -fomit-frame-pointer -Os -g -L../lib -l:libtut_common.a
LD = ld
LDFLAGS = -Tlinker.ld
ASM = nasm

OBJS = kentry.o main.o util.o gdt.o idt.o int.o isr.o paging.o mm.o kbd.o ata.o gpt.o syscall.o syscall_stub.o schedule.o ext2.o fat.o vfs.o vesa.o psf.o elf.o devfs.o ll.o dma.o sb16.o signal.o fd.o pipe.o rnd.o tmpfs.o acpi.o apic.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) -f elf64 $< -o $@

kernel.bin: $(OBJS)
	for obj in $^; do \
		objcopy -R .comment -R .note.* $$obj; \
    done
	$(LD) $(LDFLAGS) -o $@ $^ -L../lib -l:libtut_common.a

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) --oformat elf64-x86-64 -o $@ $^ -L../lib -l:libtut_common.a

clean:
	rm -f *.o *.bin *.elf
