#include "main.h"
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

void kwarn(const char * source, const char * func, const char * msg) {
    kputs("[KWRN] ");
    kputs((char*)source);
    kputc(':');
    kputs((char*)func);
    kputs("() | ");
    kputs((char*)msg);
    kputc('\n');
}

void hexdump(void * ptr, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!(i % 16)) {
            kputs("0x");
            kputhex((uint64_t)ptr+i);
            kputs(" | ");
        }

        uint8_t val = ((uint8_t*)ptr)[i];

        if (val < 0x10)
            kputc('0');
        kputhex(val);
        kputc(' ');

        if (i%16 == 15)
            kputc('\n');
    }

    if (n%16)
        kputc('\n');
}

size_t strlen(char * str) {
    size_t o = 0;
    for (;str[o];o++);
    return o;
}

void init_8042ps2() {
    // Step 1: Disable devices
    write_comm_8042ps2(0xAD);
    write_comm_8042ps2(0xA7);

    // Step 2: Clear input buffer
    if (read_status_8042ps2() & 1)
        read_data_8042ps2();

    // Step 3: Set configuration byte
    write_comm_8042ps2(0x20);
    uint8_t cfg = read_data_8042ps2();
    cfg &= ~(3 | (1<<6));

    write_comm_8042ps2(0x60);
    write_data_8042ps2(cfg);

    write_comm_8042ps2(0x20);
    cfg = read_data_8042ps2();

    if (!(cfg & (1 << 5))) {
        kwarn(__FILE__,__func__,"no dual channel ps/2");
    }

    // Step 4: Self test
    write_comm_8042ps2(0xAA);
    uint8_t st = read_data_8042ps2();
    if (st != 0x55) {
        kwarn(__FILE__,__func__,"ps/2 controller failed self test");
        return;
    }

    // Step 5: Check if there are 2 channels (again)
    write_comm_8042ps2(0xA8); // Try to enable channel two
    write_comm_8042ps2(0x20);
    cfg = read_data_8042ps2();
    if (cfg & (1 << 5)) { // Is it still disabled
        kwarn(__FILE__,__func__,"no dual channel ps/2");
    }
    write_comm_8042ps2(0xA7);

    // Step 6: Test ports
    write_comm_8042ps2(0xAB);
    st = read_data_8042ps2();
    if (st) {
        kwarn(__FILE__,__func__,"ps/2 port 1 test fail");
        return;
    }

    write_comm_8042ps2(0xA9);
    st = read_data_8042ps2();
    if (st) {
        kwarn(__FILE__,__func__,"ps/2 port 2 test fail");
        return;
    }

    // Step 7: Enable ports
    write_comm_8042ps2(0xAE);
    write_comm_8042ps2(0xA8);

    // Step 8: Reset devices
    write_ps2_port1(0xFF);
    read_data_8042ps2(); // Get ACK
    if (read_data_8042ps2() != 0xAA) {
        kwarn(__FILE__,__func__,"ps/2 port 1 device reset fail");
        return;
    }
    write_ps2_port2(0xFF);
    read_data_8042ps2(); // Get ACK
    if (read_data_8042ps2() != 0xAA) {
        kwarn(__FILE__,__func__,"ps/2 port 2 device reset fail");
        return;
    }


    // Step 9: Setup keyboard
    write_ps2_port1(0xF0); // Set scancode set
    read_data_8042ps2(); // Get ACK
    write_ps2_port1(0x02); // Scancode set 2
    read_data_8042ps2(); // Get ACK

    write_ps2_port1(0xF0);
    read_data_8042ps2(); // Get ACK
    write_ps2_port1(0x00); // Get current scan set
    read_data_8042ps2(); // Get ACK
    uint8_t r = read_data_8042ps2();
    if (r != 0x02 && r != 0x41) { // OSDev wiki says to check for 0x41, but QEMU seems to give the direct number
        kwarn(__FILE__,__func__,"ps/2 keyboard doesn't support scancode set 2");
    }

    // Step 10: Clear buffer (again)
    while (read_status_8042ps2() & 1)
        read_data_8042ps2();

    // Step 11: Enable IRQs for keyboard only
    write_comm_8042ps2(0x20);
    cfg = read_data_8042ps2();
    cfg |= 1;
    write_comm_8042ps2(0x60);
    write_data_8042ps2(cfg);
}

uint8_t read_data_8042ps2() {
    while (!(inb(0x64) & 1));

    return inb(0x60);
}

void write_data_8042ps2(uint8_t d) {
    while (inb(0x64) & 2);

    outb(0x60, d);
}

void write_comm_8042ps2(uint8_t d) {
    while (inb(0x64) & 2);

    outb(0x64, d);
}

void write_ps2_port2(uint8_t d) {
    write_comm_8042ps2(0xD4);
    write_data_8042ps2(d);
}
