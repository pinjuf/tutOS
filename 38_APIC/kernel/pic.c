#include "pic.h"
#include "util.h"

// This has been replaced by our APIC implementation,
// but we keep it around for legacy reasons

void init_pic(void) {
    // ICW1
    outb(PIC1, ICW1_INIT | ICW1_ICW4);
    outb(PIC2, ICW1_INIT | ICW1_ICW4);

    // ICW2 (offset)
    outb(PIC1_DATA, 32); // HW-Ints 0-7 mapped 32-39
    outb(PIC2_DATA, 40); // HW-Ints 8-15 mapped 40-47

    // ICW3 (cascade)
    outb(PIC1_DATA, (1 << 2)); // IRQ2 -> PIC2 (the PIC chips communicate over the IRQ2 line)
    outb(PIC2_DATA, 2); // This PIC is connected to the IRQ2 line

    // ICW4
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // OCW1 (IRQ masks)
    outb(PIC1_DATA, 0);
    outb(PIC2_DATA, 0);

    // Send an EOI to clear them, just in case
    outb(PIC1, PIC_EOI);
    outb(PIC2, PIC_EOI);
}

void pic_setmask(uint16_t mask) {
    outb(PIC1_DATA, mask & 0xFF);
    outb(PIC2_DATA, (mask >> 8) & 0xFF);
}

uint16_t pic_getmask() {
    uint16_t mask = 0;
    mask |= inb(PIC1_DATA);
    mask |= inb(PIC2_DATA) << 8;
    return mask;
}
