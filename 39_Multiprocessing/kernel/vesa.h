#pragma once

#include "types.h"
#include "psf.h"

typedef struct vbe_info_t {
	char signature[4];
	uint16_t version;
	uint32_t oem;
	uint32_t capabilities;
	uint32_t video_modes;
	uint16_t video_memory;
	uint16_t software_rev;
	uint32_t vendor;
	uint32_t product_name;
	uint32_t product_rev;
	char reserved[222];
	char oem_data[256];
} __attribute__((packed)) vbe_info_t;

typedef struct vbe_mode_info_t {
    uint16_t attributes;
	uint8_t window_a;
	uint8_t window_b;
	uint16_t granularity;
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;
	uint16_t pitch;
	uint16_t width;
	uint16_t height;
	uint8_t w_char;
	uint8_t y_char;
	uint8_t planes;
	uint8_t bpp;
	uint8_t banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t image_pages;
	uint8_t reserved0;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;

	uint32_t framebuffer;
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;
	uint8_t reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

#define VESA_VIRT_FB 0xFFFF800000000000 // TODO: Find a better position

void init_vesa();

// We assume a 32-bpp framebuffer with 0x00RRGGBB format
typedef uint32_t rgb32_t;
#define RGB32(r, g, b) ((rgb32_t)(((r) << 16) | ((g) << 8) | (b)))
#define SET_PIXEL(x, y, color) *(uint32_t*)((size_t)VESA_VIRT_FB + (y) * bpob->vbe_mode_info.pitch + (x) * sizeof(uint32_t)) = (color)
#define GET_PIXEL(x, y) (*(uint32_t*)((size_t)VESA_VIRT_FB + (y) * bpob->vbe_mode_info.pitch + (x) * sizeof(uint32_t))) // TODO: Add double buffering

rgb32_t col256_to_rgb32(uint8_t n);

void vesa_clear(rgb32_t c);
void vesa_drawrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, rgb32_t c);
void vesa_drawcircle(uint32_t x, uint32_t y, int r, rgb32_t c);
void vesa_scrolldown(uint32_t n);

extern uint32_t vheight, vwidth;
extern psf2_header_t * vfont;
extern rgb32_t vfont_fg, vfont_bg;
extern bool vesa_ready;
extern uint32_t vesa_x, vesa_y;
extern bool vesa_bold, vesa_italic, vesa_underline, vesa_dunderline, vesa_strike;
extern uint8_t vesa_utf8_expected;
extern uint32_t vesa_utf8_codepoint;

void vesa_putc(char c);
void vesa_puts(char * s);

// 1-8:  normal
// 8-16: bright
static const rgb32_t VESA_COLORS[] = {
    RGB32(0, 0, 0),       // black
    RGB32(170, 0, 0),     // red
    RGB32(0, 170, 0),     // green
    RGB32(170, 85, 0),    // yellow
    RGB32(0, 0, 170),     // blue
    RGB32(170, 0, 170),   // magenta
    RGB32(0, 170, 170),   // cyan
    RGB32(170, 170, 170), // white
    RGB32(85, 85, 85),    // bright black
    RGB32(255, 85, 85),   // bright red
    RGB32(85, 255, 85),   // bright green
    RGB32(255, 255, 85),  // bright yellow
    RGB32(85, 85, 255),   // bright blue
    RGB32(255, 85, 255),  // bright magenta
    RGB32(85, 255, 255),  // bright cyan
    RGB32(255, 255, 255)  // bright white
};
