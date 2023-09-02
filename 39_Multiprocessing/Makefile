QEMUFLAGS = -hda img -hdb c.img -m 320M -audiodev pa,id=audio0 -machine pcspk-audiodev=audio0 -audio pa,model=sb16 -smp cpus=4

.PHONY: all run debug .FORCE
.FORCE:

all: run

run: img c.img
	qemu-system-x86_64 $(QEMUFLAGS) -debugcon stdio -accel kvm

debug: img c.img kernel/kernel.elf
	qemu-system-x86_64 $(QEMUFLAGS) -s -S &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf"

bochs: img c.img
	bochs -unlock -q -log bochs.log

img: boot/boot.bin stage2/stage2.bin ap_startup/ap_startup.bin kernel/kernel.bin
	cat $^ > img
	# Add extra padding to make sure we get above the next 512 byte boundary (Bochs BIOS int 0x13 ah=0x42 won't load the last incomplete sector, but SeaBIOS will)
	dd if=/dev/zero of=img conv=notrunc oflag=append bs=512 count=1

c.img: .FORCE userland.img
	./mkgpt.sh $@ userland.img

boot/boot.bin: boot/
	$(MAKE) -C boot boot.bin

stage2/stage2.bin: stage2/
	$(MAKE) -C stage2 stage2.bin

ap_startup/ap_startup.bin: ap_startup/
	$(MAKE) -C ap_startup ap_startup.bin

kernel/kernel.bin: kernel/ lib/libtut_common.a
	$(MAKE) -C kernel kernel.bin

kernel/kernel.elf: kernel/ lib/libtut_common.a
	$(MAKE) -C kernel kernel.elf

userland.img: .FORCE userland
	dd if=/dev/zero of=$@ bs=1048576 count=32
	mkfs.ext2 -d userland $@

userland: userland/ lib/libtut_common.a userland/src/ userland/lib/
	$(MAKE) -C userland

lib/libtut_common.a: lib/
	$(MAKE) -C lib libtut_common.a
