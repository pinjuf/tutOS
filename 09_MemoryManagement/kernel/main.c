#include "main.h"
#include "paging.h"
#include "vga.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
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

    init_pic();
    kputs("PIC OK\n");

    init_pit0(PIT0_FREQ); // Interrupt every 4ms
    kputs("PIT OK\n");

    kputs("KRN MN\n");

    char * my_msg = (char*)(HEAP_VIRT + (HEAP_PTS) * 512 * 0x1000) - 3;
    my_msg[0] = '*'; // Star of life
    my_msg[1] = '\n'; // Star of life
    my_msg[2] = 0;

    kputs(my_msg);

    sti;

    kputs("KRN DN\n");
    while (1);
}
