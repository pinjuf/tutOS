#include "isr.h"
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
    if (!(pit0_ticks % 250))
        kputs("Beep! See you in 1 second...\n");
}
