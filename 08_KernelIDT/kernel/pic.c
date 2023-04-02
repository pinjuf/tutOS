#include "pic.h"
#include "util.h"

// TODO: Add DEFINE for all masks/ports/etc.
void init_pic(void) {
    // ICW1
	outb(0x20, 0x11); // 0b00010001 | edge trigger | cascade mode | ICW4 needed
	outb(0xA0, 0x11);

    // ICW2
	outb(0x21, 0x20); // 0x20 = 32  | IRQ0-7 mapped to 0x20-0x27
	outb(0xA1, 0x28); // 0x70 = 112 | IRQ8-15 mapped to 0x70-0x77

    // ICW3
	outb(0x21, 0x04); // 0b00000100 | slave PIC at IRQ2
	outb(0xA1, 0x02); // 0b00000010 | cascade identity

    // ICW4
	outb(0x21, 0x01); // 0b00000001 | not fully nested | not buffered | slave mode  | normal EOI
	outb(0xA1, 0x01); // 0b00000001 | not fully nested | not buffered | master mode | normal EOI

    // OCW1
	outb(0x21, 0x0); // 0b00000000 | no interrupts masked
	outb(0xA1, 0x0);
}
