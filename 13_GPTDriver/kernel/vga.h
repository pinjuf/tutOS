#pragma once

#include "types.h"

#define VGA_BUF 0xB8000
#define VGA_ROWS 25
#define VGA_COLS 80

enum VGA_COLOR {
    VGA_BLACK = 0,
    VGA_BLUE,
    VGA_GREEN,
    VGA_CYAN,
    VGA_RED,
    VGA_MAGENTA,
    VGA_BROWN,
    VGA_LIGHT_GREY,
    VGA_DARK_GREY,
    VGA_LIGHT_BLUE,
    VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN,
    VGA_LIGHT_RED,
    VGA_LIGHT_MAGENTA,
    VGA_LIGHT_BROWN,
    VGA_WHITE,
};

#define VGA_ATTR(fg, bg) ((fg) | (bg) << 4)
#define VGA_CHAR(c) ((c) | vga_attr << 8)

extern uint8_t vga_x, vga_y;
extern uint8_t vga_attr;

void vga_clear();
void vga_scrolldown();

void vga_putc(char c);
void vga_puts(char * s);

void vga_enable_cursor(uint8_t start, uint8_t end);
void vga_disable_cursor();
void vga_update_cursor();
void vga_get_cursor();
