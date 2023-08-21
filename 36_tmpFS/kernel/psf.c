#include "psf.h"
#include "vesa.h"
#include "util.h"
#include "main.h"

void psf2_putvesa(psf2_header_t * font, uint32_t c, uint32_t x, uint32_t y) {
    if (c >= font->length)
        return;

    char * bitmap = (char*) ((size_t)font + font->header_size + font->char_size * c);

    uint32_t bpl = font->width / 8;
    if (font->width % 8)
        bpl++;

    vesa_drawrect(x, y, font->width, font->height, vfont_bg);

    for (uint32_t dy = 0; dy < font->height; dy++) {
        char * line = (char*) ((size_t)bitmap + bpl * dy);

        // Offset for italic text (the smaller the divisor, the bigger the shift)
        uint8_t dx_offset = vesa_italic * ((font->height - dy) / 4);

        for (uint32_t dx = 0; dx < font->width; dx++) {
            if (line[dx / 8] & (1 << (7 - dx % 8))) {
                SET_PIXEL(x + dx + dx_offset, y + dy, vfont_fg);

                if (vesa_bold) { // Set the right next pixel too
                    SET_PIXEL(x + dx + dx_offset + 1, y + dy, vfont_fg);
                }
            }
        }
    }

    if (vesa_underline)
        vesa_drawrect(x, y + font->height - 1, font->width, 1, vfont_fg);
}
