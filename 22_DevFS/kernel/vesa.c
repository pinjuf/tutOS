#include "vesa.h"
#include "main.h"
#include "paging.h"
#include "util.h"
#include "vfs.h"
#include "psf.h"
#include "mm.h"
#include "kbd.h"

bool vesa_ready = false; // kputs etc. should know when it is ok to write to VESA
uint32_t vheight, vwidth;
psf2_header_t * vfont;
rgb32_t vfont_fg = RGB32(255, 255, 255);
rgb32_t vfont_bg = RGB32(0, 0, 0);

uint32_t vesa_x = 0;
uint32_t vesa_y = 0;

void init_vesa() {
    // Map the framebuffer based on the physical address in the BPOB
    size_t fb_size = bpob->vbe_mode_info.height * bpob->vbe_mode_info.pitch;

    size_t fb_pages = fb_size / PAGE_SIZE;
    if (fb_size % PAGE_SIZE) fb_pages++;

    mmap_pages((void*)VESA_VIRT_FB,
            (void*)(size_t)bpob->vbe_mode_info.framebuffer,
            PAGE_PRESENT | PAGE_RW,
            fb_pages);

    vheight = bpob->vbe_mode_info.height;
    vwidth = bpob->vbe_mode_info.width;

    filehandle_t * font_fh = kopen(VFONT, FILE_R);

    if (!font_fh)
        kwarn(__FILE__,__func__,"vfont not found");

    vfont = kmalloc(font_fh->size);
    kread(font_fh, vfont, font_fh->size);
    kclose(font_fh);

    if (vfont->magic != PSF2_MAGIC)
        kwarn(__FILE__,__func__,"wrong psf2 magic (psf1?)");

    mouse_x = vwidth >> 1;
    mouse_y = vheight >> 1;

    vesa_ready = true;
}

void vesa_clear(rgb32_t c) {
    for (uint32_t y = 0; y < vheight; y++)
        for (uint32_t x = 0; x < vwidth; x++)
            SET_PIXEL(x, y, c);
}

void vesa_drawrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, rgb32_t c) {
    for (size_t dy = 0; dy < h; dy++)
        for (size_t dx = 0; dx < w; dx++)
            SET_PIXEL(x+dx, y+dy, c);
}

void vesa_drawcircle(uint32_t x, uint32_t y, int r, rgb32_t c) {
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r)
                SET_PIXEL(x+dx, y+dy, c);
}

void vesa_scrolldown(uint32_t n) {
    for (uint32_t y = 0; y < vheight - n; y++)
        for (uint32_t x = 0; x < vwidth; x++)
            SET_PIXEL(x, y, GET_PIXEL(x, y+n));

    vesa_drawrect(0, vheight - n, vwidth, n, vfont_bg);
}

void vesa_putc(char c) {
    const uint32_t vesa_cols = vwidth / vfont->width;
    const uint32_t vesa_rows = vheight / vfont->height;

    switch (c) {
        case '\r':
            vesa_x = 0;
            break;
        case '\n':
            vesa_x = 0;
            vesa_y++;
            break;
        case '\t':
            vesa_x += 4 - (vesa_x % 4);
            break;
        case '\b':
            if (vesa_x)
                vesa_x--;
            psf2_putvesa(vfont, ' ', vesa_x * vfont->width, vesa_y * vfont->height);
            break;
        default:
            psf2_putvesa(vfont, c, vesa_x * vfont->width, vesa_y * vfont->height);
            vesa_x++;
            break;
    }

    if (vesa_x >= vesa_cols) {
        vesa_x = 0;
        vesa_y++;
    }

    if (vesa_y >= vesa_rows) {
        vesa_y = vesa_rows - 1;
        vesa_scrolldown(vfont->height);
    }
}

void vesa_puts(char * s) {
    for (; *s; s++)
        vesa_putc(*s);
}
