#include "vga.h"
#include "util.h"

uint8_t vga_x = 0, vga_y = 0;
uint8_t vga_attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);

void vga_clear(void) {
    uint16_t * buf = (uint16_t *) VGA_BUF;

    for (size_t i = 0; i < VGA_ROWS * VGA_COLS; i++) {
        *buf = VGA_CHAR(' ');
        buf++;
    }
}

void vga_putc(char c) {
    uint16_t * buf = (uint16_t *) VGA_BUF;

    switch (c) {
        case '\r':
            vga_x = 0;
            break;
        case '\n':
            vga_x = 0;
            vga_y++;
            break;
        case '\t':
            vga_x += 4 - (vga_x % 4);
            break;
        default:
            buf[vga_x + vga_y * VGA_COLS] = VGA_CHAR(c);
            vga_x++;
            break;
    }

    if (vga_x >= VGA_COLS) {
        vga_x = 0;
        vga_y++;
    }

    if (vga_y >= VGA_ROWS) {
        vga_y = VGA_ROWS - 1;
        vga_scrolldown();
    }
}

void vga_scrolldown() {
    uint16_t * buf = (uint16_t *) VGA_BUF;

    for (size_t i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++) {
        buf[i] = buf[i + VGA_COLS];
    }

    for (size_t i = (VGA_ROWS - 1) * VGA_COLS; i < VGA_ROWS * VGA_COLS; i++) {
        buf[i] = VGA_CHAR(' ');
    }
}

void vga_puts(char * s) {
    for (; *s; s++) vga_putc(*s);

    vga_update_cursor();
}

void vga_enable_cursor(uint8_t start, uint8_t end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void vga_disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_update_cursor() {
	uint16_t pos = vga_y * VGA_COLS + vga_x;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void vga_get_cursor() {
    uint16_t pos = 0;
    outb(0x3D4, 0x0F);
    pos |= inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;

    vga_y = pos / VGA_COLS;
    vga_x = pos % VGA_COLS;
}
