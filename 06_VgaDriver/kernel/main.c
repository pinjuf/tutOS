#include "vga.h"

void _kmain() {
    // Set up our stack
    asm (" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    vga_enable_cursor(0, 16);
    vga_get_cursor();

    char msg[] = "Hello,\n\tfrom the VGA driver!\n";
    vga_puts(msg);

    while (1);
}
