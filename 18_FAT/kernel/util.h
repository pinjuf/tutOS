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
int strcmp(char * s1, char * s2);
int strncmp(char * s1, char * s2, size_t n);

void itoa(uint64_t x, char * out, uint8_t base);
void sitoa(int64_t x, char * out, uint8_t base);

void kputdec(uint64_t x);
void kputhex(uint64_t x);

void hexdump(void * ptr, size_t n);

void kwarn(const char * source, const char * func, const char * msg);

void init_pit0(uint32_t freq);

void init_8042ps2();

uint8_t read_data_8042ps2();
#define read_status_8042ps2() inb(0x64)
void write_data_8042ps2(uint8_t d);
void write_comm_8042ps2(uint8_t d);

void write_ps2_port1(uint8_t d);
void write_ps2_port2(uint8_t d);

void kputleadingzeroes_hex(uint64_t val, uint8_t len);

void _rdmsr(uint32_t msr, uint32_t * lo, uint32_t * hi);
void _wrmsr(uint32_t msr, uint32_t lo, uint32_t hi);
uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t val);

#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
