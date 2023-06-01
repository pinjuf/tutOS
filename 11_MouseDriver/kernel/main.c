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

    uint8_t * buf = (uint8_t *) VGA_BUF;
    uint8_t curr = VGA_LIGHT_GREY;
    uint8_t old = VGA_BLACK;

    uint16_t x = mouse_x, y = mouse_y, old_x, old_y;

    sti;

    vga_attr = VGA_ATTR(old, old);
    vga_disable_cursor();
    vga_clear();

    while (true) {
        old_x = x;
        old_y = y;

        x = mouse_x;
        y = mouse_y;

        if (mouse_scroll == PS2_MOUSE_SCRL_DOWN) {
            curr--;
            mouse_scroll = 0;
        } else if (mouse_scroll == PS2_MOUSE_SCRL_UP) {
            curr++;
            mouse_scroll = 0;
        }

        if (old_x != x || old_y != y) {
            if (!mouse_left)
                buf[2*(old_x+VGA_COLS*old_y)+1] = VGA_ATTR(old & 0xF, old & 0xF);
            old = buf[2*(x+VGA_COLS*y)+1];
        }

        buf[2*(x+VGA_COLS*y)+1] = VGA_ATTR(curr & 0xF, curr & 0xF);

        if (mouse_right) {
            vga_clear();
            mouse_right = false;
        }
    }

    kputs("KRN DN\n");
    while (1);
}
