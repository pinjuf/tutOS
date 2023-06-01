out1.bin: main1.o syscalls.o
	ld --oformat binary main1.o syscalls.o -o out1.bin -Ttext 0x400000

main1.o: main1.c
	gcc -c main1.c -o main1.o -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Os -g
	objcopy -R .comment -R .note.* -R .debug.* main1.o

out2.bin: main2.o syscalls.o
	ld --oformat binary main2.o syscalls.o -o out2.bin -Ttext 0x400000

main2.o: main2.c
	gcc -c main2.c -o main2.o -nostdlib -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
		 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Os -g
	objcopy -R .comment -R .note.* -R .debug.* main2.o

syscalls.o: syscalls.asm
	nasm -f elf64 syscalls.asm -o syscalls.o
