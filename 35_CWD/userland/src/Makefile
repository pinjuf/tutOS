CC = gcc
CFLAGS = -T linker.ld -Wall -Wextra -Wpedantic -nostdlib -ffreestanding -fno-pie \
		 -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -L../lib -I../lib -l:libtut.a -static -Wno-unused-parameter -L../../lib \
	     -l:libtut_common.a
ASM = nasm

TARGETS = init test ls echo cat wavplay kill tree clear lolcat pwd mount

all: $(TARGETS)

.PHONY: all

$(TARGETS): % : %.o
	$(CC) $< $(CFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(ASM) -f elf64 $< -o $@

clean:
	rm -f *.o $(TARGETS)
