#include "psf.h"
#include "vesa.h"
#include "util.h"
#include "main.h"

void psf2_putvesa(psf2_header_t * font, uint8_t c, uint32_t x, uint32_t y) {
    char * bitmap = (char*) ((size_t)font + font->header_size + font->char_size * c);

    uint32_t bpl = font->width / 8;
    if (font->width % 8)
        bpl++;

    for (uint32_t dy = 0; dy < font->height; dy++) {
        char * line = (char*) ((size_t)bitmap + bpl * dy);

        for (uint32_t dx = 0; dx < font->width; dx++) {
            if (line[dx / 8] & (1 << (7 - dx % 8))) {
                SET_PIXEL(x + dx, y + dy, vfont_fg);
            } else {
                SET_PIXEL(x + dx, y + dy, vfont_bg);
            }
        }
    }
}
