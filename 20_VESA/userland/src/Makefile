test: syscalls.o test.o
	ld -o test syscalls.o test.o -Ttext 0x410000

test.o: test.c
	gcc -c test.o test.c -nostdlib -ffreestanding -fno-pie -fno-stack-protector

syscalls.o: syscalls.asm
	nasm -f elf64 syscalls.asm -o syscalls.o
