all: run

run: img
	qemu-system-x86_64 -hda img

img: boot/boot.bin stage2/stage2.bin kernel/kernel.bin
	cat boot/boot.bin stage2/stage2.bin kernel/kernel.bin > img

boot/boot.bin: boot
	$(MAKE) -C boot boot.bin

stage2/stage2.bin: stage2
	$(MAKE) -C stage2 stage2.bin

kernel/kernel.bin: kernel
	$(MAKE) -C kernel kernel.bin
