all: run

run: img
	qemu-system-x86_64 -hda img

img: boot/boot.bin stage2/stage2.bin
	cat boot/boot.bin stage2/stage2.bin > img

boot/boot.bin: boot
	$(MAKE) -C boot boot.bin

stage2/stage2.bin: stage2
	$(MAKE) -C stage2 stage2.bin
