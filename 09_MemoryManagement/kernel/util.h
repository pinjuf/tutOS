#pragma once

#include "types.h"

#define sti asm("sti")
#define cli asm("cli")

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);

void kputc(char c);
void kputs(char * s);

void qemu_putc(char c);
void qemu_puts(char * s);

void memset(void * dest, uint8_t val, size_t len);
void memcpy(void * dest, void * src, size_t len);

size_t strlen(char * str);

void itoa(uint64_t x, char * out, uint8_t base);

void kputdec(uint64_t x);
void kputhex(uint64_t x);

void hexdump(void * ptr, size_t n);

void kwarn(const char * source, const char * func, const char * msg);

void init_pit0(uint32_t freq);
