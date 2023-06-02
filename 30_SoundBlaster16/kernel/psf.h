#pragma once

#include "types.h"

typedef struct psf2_header_t {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t length;
    uint32_t char_size;
    uint32_t height;
    uint32_t width;
} __attribute__((packed)) psf2_header_t;

#define PSF2_MAGIC 0x864ab572

#define VFONT "/etc/vfont.psf2"

void psf2_putvesa(psf2_header_t * font, uint8_t c, uint32_t x, uint32_t y);
