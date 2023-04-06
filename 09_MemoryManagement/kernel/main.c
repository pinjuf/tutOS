#include "main.h"
#include "paging.h"
#include "vga.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"

void _kmain() {
    // Set up our stack
    asm (" \
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

    kputs("KRN MN\n");

    void * a = alloc_pages(2);
    void * b = kmalloc(0x100);
    void * c = kmalloc(0x200);

    kfree(b);

    void * d = kmalloc(0x100);

    kputs("A=0x");
    kputhex((uint64_t)a);
    kputs("\nB=0x");
    kputhex((uint64_t)b);
    kputs("\nC=0x");
    kputhex((uint64_t)c);
    kputs("\nD=0x");
    kputhex((uint64_t)d);
    kputc('\n');

    sti;

    kputs("KRN DN\n");
    while (1);
}
