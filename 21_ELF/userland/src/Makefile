test: syscalls.o test.o
	ld -o test syscalls.o test.o

test.o: test.c
	gcc -c test.o test.c -nostdlib -fno-pie -ffreestanding -fno-stack-protector

syscalls.o: syscalls.asm
	nasm -f elf64 syscalls.asm -o syscalls.o
