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

void memset(void * dest, uint8_t val, size_t len) {
    uint8_t *dst = (uint8_t *)dest;
    for (; len != 0; len--) *dst++ = val;
}

void memcpy(void * dest, void * src, size_t len) {
    const uint8_t * s = (const uint8_t *)src;
    uint8_t * d = (uint8_t *)dest;
    for (; len != 0; len--) *d++ = *s++;
}

