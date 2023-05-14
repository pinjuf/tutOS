#include "vesa.h"
#include "main.h"
#include "paging.h"
#include "util.h"
#include "vfs.h"
#include "psf.h"
#include "mm.h"

uint32_t vheight, vwidth;
psf2_header_t * vfont;
rgb32_t vfont_fg = RGB32(255, 255, 255);
rgb32_t vfont_bg = RGB32(0, 0, 0);

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

    // Quick demo
    for (size_t i = 0; i < 10; i++)
        psf2_putvesa(vfont, '0' + i, vfont->width * i, 0);
    for (size_t i = 0; i < 26; i++)
        psf2_putvesa(vfont, 'A' + i, vfont->width * i, vfont->height);
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
