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

    kputs("KRN MN\n");

    part_t * usercode_part = get_part(1, 0);
    size_t usercode_pages = usercode_part->size / 2;
    if (usercode_part->size & 1) usercode_pages++;
    void * usercode = calloc_pages(usercode_pages);

    mmap_pages((void*)0x400000, virt_to_phys(usercode), PAGE_PRESENT | PAGE_RW | PAGE_USER, usercode_pages);
    while (part_read(usercode_part, 0, usercode_part->size * 512, (void*)0x400000));

    void * userstack = (void*)((uint64_t)calloc_pages(1) + PAGE_SIZE * 1);

    asm volatile(" \
            push $0x2B; \
            push %0; \
            push $0x202; \
            push $0x33; \
            push %1; \
            mov %0, %%rbp ; \
            iretq;" :: "r"(userstack), "r"((void*)0x400000));

    kputs("KRN DN\n");
    while (1);
}
