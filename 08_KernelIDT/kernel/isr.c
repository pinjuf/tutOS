#include "isr.h"
#include "util.h"

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;

    kputs("Exception!\n");

    while (1);
}

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp) {
    (void)n;
    (void)err;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;

    kputs("Error exception!\n");

    while (1);
}

void isr_default_int(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;

    kputs("Interrupt!\n");

    while (1);
}
