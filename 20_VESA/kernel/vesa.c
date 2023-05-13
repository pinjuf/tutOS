#include "vesa.h"
#include "main.h"
#include "paging.h"
#include "util.h"

uint32_t vheight, vwidth;

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
