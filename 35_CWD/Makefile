QEMUFLAGS = -hda img -hdb c.img -m 320M -audiodev pa,id=audio0 -machine pcspk-audiodev=audio0 -audio pa,model=sb16

.PHONY: all run debug .FORCE
.FORCE:

all: run

run: img c.img
	qemu-system-x86_64 $(QEMUFLAGS) -debugcon stdio -accel kvm

debug: img c.img kernel/kernel.elf
	qemu-system-x86_64 $(QEMUFLAGS) -s -S &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf"

bochs: img c.img
	bochs -q

img: boot/boot.bin stage2/stage2.bin kernel/kernel.bin
	cat boot/boot.bin stage2/stage2.bin kernel/kernel.bin > img

c.img: .FORCE userland.img
	./mkgpt.sh $@ userland.img

boot/boot.bin: boot/
	$(MAKE) -C boot boot.bin

stage2/stage2.bin: stage2/
	$(MAKE) -C stage2 stage2.bin

kernel/kernel.bin: kernel/ lib/libtut_common.a
	$(MAKE) -C kernel kernel.bin

kernel/kernel.elf: kernel/ lib/libtut_common.a
	$(MAKE) -C kernel kernel.elf

userland.img: .FORCE userland
	dd if=/dev/zero of=$@ bs=1048576 count=256
	mkfs.ext2 -d userland $@

userland: userland/ lib/libtut_common.a userland/src/ userland/lib/
	$(MAKE) -C userland

lib/libtut_common.a: lib/
	$(MAKE) -C lib libtut_common.a
