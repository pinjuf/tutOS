#include "util.h"
#include "vga.h"

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void qemu_putc(char c) {
    if (c == '\n') outb(0xe9, '\r');
    outb(0xe9, c);
}

void qemu_puts(char * s) {
    for (; *s; s++) qemu_putc(*s);
}

void kputc(char c) {
    vga_putc(c);
    vga_update_cursor();
    qemu_putc(c);
}

void kputs(char * s) {
    vga_puts(s);
    qemu_puts(s);
}

void kputdec(uint64_t x) {
    char buf[32];
    itoa(x, buf, 10);
    kputs(buf);
}

void kputhex(uint64_t x) {
    char buf[32];
    itoa(x, buf, 16);
    kputs(buf);
}


void memset(void * dest, uint8_t val, size_t len) {
    uint8_t *dst = (uint8_t *)dest;
    for (; len != 0; len--) *dst++ = val;
}

void memcpy(void * dest, void * src, size_t len) {
    const uint8_t * s = (const uint8_t *)src;
    uint8_t * d = (uint8_t *)dest;
    for (; len != 0; len--) *d++ = *s++;
}

void itoa(uint64_t x, char * out, uint8_t base) {
    char * p = out;
    char * low = out;

    do {
        *p++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[x % base];
        x /= base;
    } while (x);

    *p-- = '\0';

    while (low < p) {
        char tmp = *low;
        *low++ = *p;
        *p-- = tmp;
    }
}

void init_pit0(uint32_t freq) {
    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0x36); // Configure mode for PIT0
    outb(0x40, divisor & 0xFF); // Low PIT0 byte
    outb(0x40, (divisor >> 8) & 0xFF); // High PIT0 byte
}
