#include "main.h"
#include "paging.h"
#include "vga.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"
#include "ata.h"
#include "kbd.h"

void _kmain() {
    // Set up our stack
    asm volatile(" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    // Set up VGA
    vga_enable_cursor(0, 16);
    vga_get_cursor();

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

    init_pit0(PIT0_FREQ); // Interrupt every 4ms
    kputs("PIT OK\n");

    init_8042ps2();
    kputs("PS2 OK\n");

    pic_setmask(pic_getmask() | (3<<14)); // Disable Both ATA interrupts
    for (uint8_t i = 0; i < 8; i++) {
        ata_resetdrive(i);
    }
    ata_checkdrives();
    kputs("ATA OK\n");

    kputs("KRN MN\n");
    sti;

    for (uint8_t i = 0; i < ATA_DRIVES; i++) {
        if (drive_bitmap & (1 << i)) {
            kputs("Drive #");
            kputdec(i);
            kputs(" detected!\n");
        }
    }

    // We assume that: Drive #0 = boot drive | #1 = data drive
    char * buf = (char *)kmalloc(0x200 * 64);
    while (drive_read(0, 0, 512, buf));
    kputs("Dumping own boot sector:\n");
    hexdump(buf, 512);

    kputs("\nWriting test pattern to drive #1...");
    for (size_t i = 1; i <= 64; i++) {
        memset(buf, 0, 0x200*64);
        for (size_t j = 0; j < i; j++) {
            buf[j] = i;
        }

        while (drive_write(1, (i*(i-1))/2, i, buf));
    }
    kputs(" done (run \"hexdump -C c.img\")\n");

    cli;
    kputs("KRN DN\n");
    while (1);
}
