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

    sti;

    kputs("Opening /dev/pcspk... ");
    filehandle_t * pcspk = kopen("/dev/pcspk", FILE_W);
    kputs("done.\n");
    kputs("Beeping... ");
    uint32_t freq = 440;
    kwrite(pcspk, &freq, 4);
    size_t now = pit0_ticks;
    while (now + PIT0_FREQ > *(size_t volatile *)&pit0_ticks);
    freq = 0;
    kwrite(pcspk, &freq, 4);
    kclose(pcspk);
    kputs("done.\n");

    kputs("Opening /dev/vesafb... ");
    filehandle_t * vesafb = kopen("/dev/vesafb", FILE_W);
    kputs("done.\n");
    kputs("Making green screen... ");
    rgb32_t green = RGB32(0, 255, 0);
    for (size_t i = 0; i < vesafb->size/sizeof(rgb32_t); i++) {
        kwrite(vesafb, &green, sizeof(rgb32_t));
    }
    kclose(vesafb);
    kputs("done.\n");

    kputs("Opening /dev/tty...");
    filehandle_t * tty_r = kopen("/dev/tty", FILE_R);
    filehandle_t * tty_w = kopen("/dev/tty", FILE_W);
    kputs("done.\n");
    kputs("You may write 10 characters. No more, no less.\n> ");
    for (size_t i = 0; i < 10; i++) {
        char in;
        kread(tty_r, &in, 1);
        kwrite(tty_w, &in, 1);
    }
    kclose(tty_r);
    kclose(tty_w);
    kputs("\n");

    kputs("Opening /dev/hdb1...");
    filehandle_t * hdb1 = kopen("/dev/hdb1", FILE_R);
    kputs("done.\n");
    kputs("Dumping superblock...\n");
    hdb1->curr = SECTOR_SIZE * 2;
    char * hdb1_s1 = kmalloc(SECTOR_SIZE * 2);
    kread(hdb1, hdb1_s1, SECTOR_SIZE * 2);
    hexdump(hdb1_s1, SECTOR_SIZE * 2);
    kclose(hdb1);
    kfree(hdb1_s1);
    kputs("done.\n");

    kputs("Opening /dev/mem...");
    filehandle_t * mem = kopen("/dev/mem", FILE_R);
    kputs("done.\n");
    kputs("Dumping GDT/IDT...\n");
    char * mem_buf = kmalloc(SECTOR_SIZE * 2);
    kread(mem, mem_buf, SECTOR_SIZE * 2);
    hexdump(mem_buf, SECTOR_SIZE * 2);
    kclose(mem);
    kfree(mem_buf);
    kputs("done.\n");

    kputs("KRN DN\n");
    while (1);
}
