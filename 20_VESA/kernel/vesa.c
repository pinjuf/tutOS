#include "vesa.h"
#include "main.h"
#include "paging.h"
#include "util.h"

void init_vesa() {
    size_t fb_size = bpob->vbe_mode_info.height * bpob->vbe_mode_info.pitch;

    size_t fb_pages = fb_size / PAGE_SIZE;
    if (fb_size % PAGE_SIZE) fb_pages++;

    mmap_pages((void*)VESA_VIRT_FB,
            (void*)(size_t)bpob->vbe_mode_info.framebuffer,
            PAGE_PRESENT | PAGE_RW,
            fb_pages);
}

void vesa_clear(rgb32_t c) {
    for (size_t y = 0; y < bpob->vbe_mode_info.height; y++)
        for (size_t x = 0; x < bpob->vbe_mode_info.width; x++)
            SET_PIXEL(x, y, c);
}
