#pragma once

#include "types.h"

#define sti asm volatile ("sti")
#define cli asm volatile ("cli")

#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x > y ? y : x)

#define bochs_bp asm volatile ("xchg %bx, %bx")

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

void memmove(void * dest, void * src, size_t len);

size_t strlen(char * str);
int strcmp(char * s1, char * s2);
int strncmp(char * s1, char * s2, size_t n);
size_t strmatchstart(char * s1, char * s2);
char * strcpy(char * dst, char * src);
char * stpcpy(char * dst, char * src);

void itoa(uint64_t x, char * out, uint8_t base);
void sitoa(int64_t x, char * out, uint8_t base);

size_t atoi(char * s, uint8_t base);

void kputdec(uint64_t x);
void kputhex(uint64_t x);

void hexdump(void * ptr, size_t n);

void kwarn(const char * source, const char * func, const char * msg);

void init_pit0(uint32_t freq);
void init_pit2(uint32_t freq);

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

void kprintf(char * fmt, ...);

int memcmp(void * s1, void * s2, size_t n);

char * strstr(char * haystack, char * needle);

size_t strreplace(char * str, char * find, char * replace);

void cpuid(uint32_t code_a, uint32_t code_c, uint32_t * a, uint32_t * b, uint32_t * c, uint32_t * d);

size_t chksum8(void * ptr, size_t count);

inline __attribute__((always_inline)) uint64_t rdtsc() {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

inline __attribute__((always_inline)) uint64_t * get_pml4t() {
    // Warning: returns a PHYSICAL address
    uint64_t * pml4t;
    asm volatile("mov %%cr3, %0" : "=r"(pml4t));
    return (uint64_t*)((uint64_t)pml4t & ~0xFFF);
}

size_t get_cpu_tps();
void usleep(size_t us);

typedef volatile uint16_t spinlock_t;
void spinlock_acquire(spinlock_t * lock);
void spinlock_acquireid(spinlock_t * lock, uint16_t id);
void spinlock_release(spinlock_t * lock);

#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
