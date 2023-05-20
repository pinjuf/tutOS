#include "main.h"
#include "paging.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"
#include "ata.h"
#include "kbd.h"
#include "gpt.h"
#include "syscall.h"
#include "schedule.h"
#include "vfs.h"
#include "vesa.h"
#include "elf.h"
#include "isr.h"

bpob_t * bpob = (void*)BPOB_ADDR;

__attribute__((noreturn))
void _kmain() {
    // Set up our stack
    asm volatile(" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    // I really like cool sounding logs
    kputs("KRN OK\n");

    init_kgdt();
    kputs("GDT OK\n");

    init_idt();
    kputs("IDT OK\n");

    init_paging();
    kputs("PGN OK\n");

    init_mm();
    kputs("MM  OK\n");

    init_pic();
    kputs("PIC OK\n");

    init_pit0(PIT0_FREQ);
    kputs("PIT OK\n");

    init_8042ps2();
    kputs("PS2 OK\n");

    pic_setmask(pic_getmask() | (3<<14)); // Disable Both ATA interrupts
    for (uint8_t i = 0; i < 8; i++) {
        ata_resetdrive(i);
    }
    ata_checkdrives();
    kputs("ATA OK\n");

    init_syscalls();
    kputs("SCL OK\n");

    init_scheduling();
    kputs("SCD OK\n");

    init_vfs();
    kputs("VFS OK\n");

    init_vesa();
    vesa_clear(RGB32(0, 0, 0));
    kputs("VBE OK\n");

    kputs("KRN MN\n");

    do_scheduling = false;

    sti;

    filehandle_t * out = kopen("/dev/tty", FILE_W);
    filehandle_t * in = kopen("/dev/tty", FILE_R);

    char last = 0;
    while (last != '\n') {
        kread(in, &last, 1);
        kwrite(out, &last, 1);
    }

    kclose(out);
    kclose(in);

    kputs("KRN DN\n");
    while (1);
}
