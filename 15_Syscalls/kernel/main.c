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

void usermode_code() {
    asm volatile ("syscall");
    asm volatile ("syscall");
    while (1);
}

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

    void * code_buf = alloc_pages(1);
    void * stack_buf = alloc_pages(1);

    memcpy(code_buf, (void*)(uint64_t)usermode_code, 0x1000);

    asm volatile(" \
        push $0x2B; \
        push %0; \
        push $0x202; \
        push $0x33; \
        push %1; \
        iretq;"
        :: "r"(stack_buf), "r"(code_buf)
    );

    cli;
    kputs("KRN DN\n");
    while (1);
}
