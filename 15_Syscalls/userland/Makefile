out.bin: main.o syscalls.o
	ld --oformat binary main.o syscalls.o -o out.bin -Ttext 0x400000

main.o: main.c
	gcc -c main.c -o main.o -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Os -g
	objcopy -R .comment -R .note.* -R .debug.* main.o

syscalls.o: syscalls.asm
	nasm -f elf64 syscalls.asm -o syscalls.o
