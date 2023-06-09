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
#include "gpt.h"

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

    part_t * p = get_part(1, 0); // Disk #1, partition #1

    kputs("Starting LBA: ");
    kputdec(p->start_lba);
    kputs("\nSize: ");
    kputdec(p->size * SECTOR_SIZE);
    kputs("\nGUID:\n");
    kputguid(p->guid);
    kputs("\nType GUID:\n");
    kputguid(p->type);
    kputs("\nReading from this partition:\n");
    void * buf = kmalloc(512);
    part_read(p, 0, 512, buf);
    hexdump(buf, 512);

    cli;
    kputs("KRN DN\n");
    while (1);
}
