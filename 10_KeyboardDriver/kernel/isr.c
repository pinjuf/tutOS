#include "isr.h"
#include "kbd.h"
#include "main.h"
#include "util.h"

uint64_t pit0_ticks = 0;

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;

    kputs("EXC ");
    kputdec(n);
    kputs(" AT 0x");
    kputhex(rip);
    kputs("\n");

    while (1);
}

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp) {
    (void)n;
    (void)err;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;

    kputs("EXC ");
    kputdec(n);
    kputs(" (ERR=0x");
    kputhex(err);
    kputs(") AT 0x");
    kputhex(rip);
    kputs("\n");

    while (1);
}

void isr_default_int(uint16_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;

    kputs("INT ");
    if (n == 0xFFFF)
        kputs("[UNKN]");
    else
        kputdec(n);
    kputs(" AT 0x");
    kputhex(rip);
    kputs("\n");
}

void isr_irq0(void) {
    pit0_ticks++;
}

void isr_irq1(void) {
    // The keyboard uses SCS2, but the 8042 translates it to SCS1 for us
    uint8_t c = read_data_8042ps2();
    bool release = false;
    bool special = false;

    if (c == 0x00 || // Key detection error/internal buffer overrun
        c == 0xAA || // Self test pass
        c == 0xEE || // Echo
        c == 0xFA || // ACK
        c == 0xFC || // Self test fail
        c == 0xFD || // Self test fail
        c == 0xFE || // Resend
        c == 0xFF)   // Key detection error/internal buffer overrun
    {
        while (read_status_8042ps2() & 1)
            read_data_8042ps2();

        return;
    }

    if (c == 0xE0) { // Special key
        special = true;
        c = read_data_8042ps2();
    }

    if (c & 0x80) {
        release = true;
        c &= ~0x80;
    }

    kputs("KBD-INT: ");
    kputhex(c);
    kputc(' ');
    if (special) {
        kputs("(SPC) ");
    }
    if (release) {
        kputs("(RLS)");
    }
    kputc('\n');

    // Update the scancode-kbd-bitmap

    kbd_setkey(c + special * 256, !release);
    hexdump(kbd_bitmap, 64);
}
