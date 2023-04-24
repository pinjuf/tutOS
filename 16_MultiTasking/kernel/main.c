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
#include "syscall.h"
#include "schedule.h"

__attribute__((noreturn))
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

    init_syscalls();
    kputs("SCL OK\n");

    init_scheduling();
    kputs("SCD OK\n");

    kputs("KRN MN\n");

    part_t * usercode1_part = get_part(1, 0);
    size_t usercode1_pages = usercode1_part->size / 2;
    if (usercode1_part->size & 1) usercode1_pages++;
    void * usercode1 = calloc_pages(usercode1_pages);
    while (part_read(usercode1_part, 0, usercode1_part->size * 512, usercode1));
    void * userstack1 = (void*)((uint64_t)calloc_pages(1) + PAGE_SIZE * 1);

    pagemap_t * usercode1_pagemaps = (pagemap_t*)kmalloc(sizeof(pagemap_t) * 1);
    usercode1_pagemaps[0].phys = virt_to_phys(usercode1);
    usercode1_pagemaps[0].virt = (void*)0x400000;
    usercode1_pagemaps[0].attr = PAGE_PRESENT | PAGE_RW | PAGE_USER;
    usercode1_pagemaps[0].n    = usercode1_pages;

    processes[0].state = PROCESS_NOT_RUNNING;
    processes[0].pagemaps_n = 1;
    processes[0].pagemaps = usercode1_pagemaps;
    processes[0].regs.rip = 0x400000;
    processes[0].regs.cs = 0x33;
    processes[0].regs.rflags = 0x202;
    processes[0].regs.rsp = (uint64_t)userstack1;
    processes[0].regs.ss = 0x2B;
    processes[0].regs.cr3 = 0x7D000; // TODO: get this from current cr3

    part_t * usercode2_part = get_part(1, 1);
    size_t usercode2_pages = usercode2_part->size / 2;
    if (usercode2_part->size & 1) usercode2_pages++;
    void * usercode2 = calloc_pages(usercode2_pages);
    while (part_read(usercode2_part, 0, usercode2_part->size * 512, usercode2));
    void * userstack2 = (void*)((uint64_t)calloc_pages(1) + PAGE_SIZE * 1);

    pagemap_t * usercode2_pagemaps = (pagemap_t*)kmalloc(sizeof(pagemap_t) * 1);
    usercode2_pagemaps[0].phys = virt_to_phys(usercode2);
    usercode2_pagemaps[0].virt = (void*)0x400000;
    usercode2_pagemaps[0].attr = PAGE_PRESENT | PAGE_RW | PAGE_USER;
    usercode2_pagemaps[0].n    = usercode2_pages;

    processes[1].state = PROCESS_NOT_RUNNING;
    processes[1].pagemaps_n = 1;
    processes[1].pagemaps = usercode2_pagemaps;
    processes[1].regs.rip = 0x400000;
    processes[1].regs.cs = 0x33;
    processes[1].regs.rflags = 0x202;
    processes[1].regs.rsp = (uint64_t)userstack2;
    processes[1].regs.ss = 0x2B;
    processes[1].regs.cr3 = 0x7D000;

    sti;

    while (1);
}
