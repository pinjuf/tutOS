#include "isr.h"
#include "kbd.h"
#include "main.h"
#include "schedule.h"
#include "util.h"

uint64_t pit0_ticks = 0;

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;
    (void)ss;

    kputs("EXC ");
    kputdec(n);
    kputs(" AT 0x");
    kputhex(rip);
    kputs("\n");

    while (1);
}

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss) {
    (void)n;
    (void)err;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;
    (void)ss;

    kputs("EXC ");
    kputdec(n);
    kputs(" (ERR=0x");
    kputhex(err);
    kputs(") AT 0x");
    kputhex(rip);
    kputs("\n");

    while (1);
}

void isr_default_int(uint16_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;
    (void)ss;

    kputs("INT ");
    if (n == 0xFFFF)
        kputs("[UNKN]");
    else
        kputdec(n);
    kputs(" AT 0x");
    kputhex(rip);
    kputs("\n");
}

void isr_irq0(void * regframe) {
    pit0_ticks++;

    if (!(pit0_ticks % TICKS_PER_SCHEDULE)) {
        schedule(regframe);
    }
}

void isr_irq1(void) {
    // The keyboard uses SCS2, but the 8042 translates it to SCS1 for us

    // Because we read multiple times in this ISR, the 8042 will trigger
    // multiple interrupts WHILE we are in this ISR, even when we have
    // already read the data corresponding to the IRQ. Therefore, the
    // moment we send the EOI, the 8259 will send the next IRQ.
    // So, we need to check if there really is data to be read.
    if (!(read_status_8042ps2() & 1)) // Is there even data to be read?
        return;

    uint8_t c = read_data_8042ps2();
    bool release = false;
    bool special = false;

    // Translation may cause data mangling, which SHOULDN'T be a problem
    if (c == 0x00) // || // Key detection error/internal buffer overrun
        //c == 0xAA || // Self test pass
        //c == 0xEE || // Echo
        //c == 0xFA || // ACK
        //c == 0xFC || // Self test fail
        //c == 0xFD || // Self test fail
        //c == 0xFE || // Resend
        //c == 0xFF)   // Key detection error/internal buffer overrun
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

    // Update the scancode-kbd-bitmap
    kbd_setkey(c + special * 256, !release);

    // Update the last-pressed-ascii-key-handle
    if (!(release || special)) {
        bool is_shift = kbd_getkey(SCS1_LSHIFT) || kbd_getkey(SCS1_RSHIFT);

        if (scancode_to_ascii(c)) {
            kbd_last_ascii = is_shift ? scancode_shift_to_ascii(c) : scancode_to_ascii(c);
        }
    }

    if (!release) {
        kbd_last_scancode = c + special * 256;
    }

    // Just to be sure, clear the 8042 output buffer
    while (read_status_8042ps2() & 1)
        read_data_8042ps2();
}

void isr_irq12(void) {
    if (!(read_status_8042ps2() & 1)) // Is there even data to be read?
        return;
    uint8_t mouse_flags = read_data_8042ps2();

    if (!(read_status_8042ps2() & 1)) // Make sure the X info isn't missing
        return;
    uint8_t x_raw = read_data_8042ps2();

    if (!(read_status_8042ps2() & 1)) // Make sure the Y info isn't missing
        return;
    uint8_t y_raw = read_data_8042ps2();

    if (!(read_status_8042ps2() & 1)) // Make sure the Z (scroll) info isn't missing
        return;
    uint8_t scroll_raw = read_data_8042ps2() & 0xF; // Drop top 4 bits

    int16_t true_dx = x_raw;
    int16_t true_dy = y_raw;

    if (mouse_flags & PS2_MOUSE_XSIGN)
        true_dx -= 256;
    if (mouse_flags & PS2_MOUSE_YSIGN)
        true_dy -= 256;

    mouse_left = (mouse_flags & PS2_MOUSE_LEFT) != 0;
    mouse_middle = (mouse_flags & PS2_MOUSE_MDDL) != 0;
    mouse_right = (mouse_flags & PS2_MOUSE_RGHT) != 0;

    mouse_x += true_dx * MOUSE_XSCALE;
    mouse_y += true_dy * MOUSE_YSCALE;

    if (mouse_x > MOUSE_XLIM)
        mouse_x = MOUSE_XLIM;
    if (mouse_y > MOUSE_YLIM)
        mouse_y = MOUSE_YLIM;

    if (mouse_x < 0)
        mouse_x = 0;
    if (mouse_y < 0)
        mouse_y = 0;

    if (scroll_raw)
        mouse_scroll = scroll_raw;

    while (read_status_8042ps2() & 1)
        read_data_8042ps2();
}
