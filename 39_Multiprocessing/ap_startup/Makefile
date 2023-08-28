all: ap_startup.bin

ap_startup.bin: ap_startup.asm
	nasm -f bin ap_startup.asm -o ap_startup.bin

clean:
	rm -f ap_startup.bin
