#include "vga.h"
#include "util.h"
#include "gdt.h"

void _kmain() {
    // Set up our stack
    asm (" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    // Set up VGA
    vga_enable_cursor(0, 16);
    vga_get_cursor();

    kputs("KRN OK\n");

    init_kgdt();
    kputs("GDT OK\n");

    kputs("KRN DN\n");
    while (1);
}
