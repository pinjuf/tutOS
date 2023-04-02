#include "vga.h"
#include "util.h"
#include "gdt.h"

void _kmain() {
    // Set up our stack
    asm (" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    vga_enable_cursor(0, 16);
    vga_get_cursor();

    init_kgdt();
    kputs("GDT OK\n");

    while (1);
}
