QEMUFLAGS = -hda img -hdb c.img -m 320M -audiodev pa,id=audio0 -machine pcspk-audiodev=audio0

.PHONY: all run debug .FORCE
.FORCE:

all: run

run: img c.img
	qemu-system-x86_64 $(QEMUFLAGS) -debugcon stdio

debug: img c.img kernel/kernel.elf
	qemu-system-x86_64 $(QEMUFLAGS) -s -S &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf"

bochs: img c.img
	bochs -q

img: boot/boot.bin stage2/stage2.bin kernel/kernel.bin
	cat boot/boot.bin stage2/stage2.bin kernel/kernel.bin > img

c.img: .FORCE userland/out.bin
	./mkgpt.sh $@ userland/out.bin

boot/boot.bin: boot
	$(MAKE) -C boot boot.bin

stage2/stage2.bin: stage2
	$(MAKE) -C stage2 stage2.bin

kernel/kernel.bin: kernel
	$(MAKE) -C kernel kernel.bin

kernel/kernel.elf: kernel
	$(MAKE) -C kernel kernel.elf

userland/out.bin: userland
	$(MAKE) -C userland out.bin
