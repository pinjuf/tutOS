QEMUFLAGS = -hda img -debugcon stdio -m 320M

all: run

run: img
	qemu-system-x86_64 $(QEMUFLAGS)

debug: img kernel/kernel.elf
	qemu-system-x86_64 $(QEMUFLAGS) -s -S &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf"

img: boot/boot.bin stage2/stage2.bin kernel/kernel.bin
	cat boot/boot.bin stage2/stage2.bin kernel/kernel.bin > img

boot/boot.bin: boot
	$(MAKE) -C boot boot.bin

stage2/stage2.bin: stage2
	$(MAKE) -C stage2 stage2.bin

kernel/kernel.bin: kernel
	$(MAKE) -C kernel kernel.bin

kernel/kernel.elf: kernel
	$(MAKE) -C kernel kernel.elf
