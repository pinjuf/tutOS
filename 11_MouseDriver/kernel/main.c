#include "main.h"
#include "paging.h"
#include "vga.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"
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

    kputs("KRN MN\n");

    vga_clear();
    vga_disable_cursor();

    sti;

    uint8_t x = mouse_x;
    uint8_t y = mouse_y;
    uint8_t * buf = (uint8_t *) VGA_BUF;
    while (true) {
        x = mouse_x;
        y = mouse_y;
        uint8_t attr = VGA_ATTR(VGA_DARK_GREY, VGA_DARK_GREY);
        if (mouse_left) {
            attr = VGA_ATTR(VGA_WHITE, VGA_WHITE);
        } else if (mouse_middle) {
            attr = VGA_ATTR(VGA_RED, VGA_RED);
        } else if (mouse_right) {
            attr = VGA_ATTR(VGA_LIGHT_GREY, VGA_LIGHT_GREY);
        }
        buf[2*(x+VGA_COLS*y) + 1] = attr;
    }

    kputs("KRN DN\n");
    while (1);
}
