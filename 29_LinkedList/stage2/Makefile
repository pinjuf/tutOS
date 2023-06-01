all: stage2.bin

stage2.bin: stage2.asm
	nasm -f bin stage2.asm -o stage2.bin

clean:
	rm -f stage2.bin
