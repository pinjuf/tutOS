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

bool vesa_bold      = false;
bool vesa_italic    = false;
bool vesa_underline = false;

bool vesa_esc       = false;
bool vesa_csi       = false;

uint16_t vesa_csi_params[8];
uint16_t vesa_csi_params_n  = 0;
char vesa_csi_parambuf[8]   = {0};

// Stored SCO positions
uint32_t vesa_sco_x = 0;
uint32_t vesa_sco_y = 0;

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

    filehandle_t * font_fh = kopen(VFONT, O_RDONLY);

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

rgb32_t col256_to_rgb32(uint8_t n) {
    rgb32_t out;

    if (n < 16)
        out = VESA_COLORS[n];
    else if (n >= 16 && n <= 231) {
        // 6x6x6 "cube" of 216 colours, see https://en.wikipedia.org/wiki/ANSI_escape_code
        n -= 16;
        uint8_t r = n / 36;
        uint8_t g = (n - r * 36) / 6;
        uint8_t b = (n - r * 36 - g * 6);

        out = RGB32(r * 51, g * 51, b * 51);
    } else {
        // grayscale colours in 24 steps
        n -= 232;

        out = RGB32(255 * n/24, 255 * n/24, 255 * n/24);
    }

    return out;
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

    if (vesa_csi) {
        // We are engaged in a special control sequence

        if (c >= '0' && c <= '9') { // Integer parameter
            vesa_csi_parambuf[strlen((char*)vesa_csi_parambuf)] = c;

        } else if (c == ';') { // Semicolon separator

            if (strlen((char*)vesa_csi_parambuf) == 0) {
                vesa_csi_params[vesa_csi_params_n++] = 0;
            } else {
                vesa_csi_params[vesa_csi_params_n++] = atoi(vesa_csi_parambuf, 10);
            }

            memset(vesa_csi_parambuf, 0, sizeof(vesa_csi_parambuf));

        } else if (c >= 0x40 && c <= 0x7E) { // Final byte
            vesa_esc = vesa_csi = false;

            // There may be an unseparated parameter
            if (strlen((char*)vesa_csi_parambuf)) {
                vesa_csi_params[vesa_csi_params_n++] = atoi(vesa_csi_parambuf, 10);
            }

            switch (c) {
                case 'A': { // Cursor up
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    if (vesa_y >= n)
                        vesa_y -= n;
                    break;
                }
                case 'B': { // Cursor down
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    if (vesa_y + n < vesa_rows)
                        vesa_y += n;
                    break;
                }
                case 'C': { // Cursor right
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    if (vesa_x + n < vesa_cols)
                        vesa_x += n;
                    break;
                }
                case 'D': { // Cursor left
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    if (vesa_x >= n)
                        vesa_x -= n;
                    break;
                }
                case 'E': { // Cursor next line
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    vesa_x = 0;
                    if (vesa_y + n < vesa_rows)
                        vesa_y += n;
                    break;
                }
                case 'F': { // Cursor previous line
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    vesa_x = 0;
                    if (vesa_y >= n)
                        vesa_y -= n;
                    break;
                }
                case 'G': { // Cursor horizontal absolute
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    if (n <= vesa_cols)
                        vesa_x = n - 1;
                    break;
                }
                case 'H': { // Cursor position
                    uint16_t y = 1;
                    uint16_t x = 1;
                    if (vesa_csi_params_n) y = vesa_csi_params[0];
                    if (vesa_csi_params_n > 1) x = vesa_csi_params[1];
                    if (x <= vesa_cols && y <= vesa_rows) {
                        vesa_x = x - 1;
                        vesa_y = y - 1;
                    }
                    break;
                }
                case 'J': { // Erase in display
                    uint16_t n = 0;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];
                    switch (n) {
                        case 0:
                            // From cursor to bottom
                            vesa_drawrect(0, vesa_y * vfont->height, vwidth, vheight - (vesa_y * vfont->height), vfont_bg);

                            break;
                        case 1:
                            // From top to cursor
                            vesa_drawrect(0, 0, vwidth, vesa_y * vfont->height, vfont_bg);

                            break;
                        case 2:
                        case 3:
                            // Entire screen
                            vesa_clear(vfont_bg);

                            break;
                        default:
                            break;
                    }

                    break;
                }
                case 'T': {
                    uint16_t n = 1;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];

                    vesa_scrolldown(n * vfont->height);
                    break;
                }
                case 's': {
                    vesa_sco_x = vesa_x;
                    vesa_sco_y = vesa_y;

                    break;
                }
                case 'u': {
                    vesa_x = vesa_sco_x;
                    vesa_y = vesa_sco_y;

                    break;
                }
                case 'm': {
                    uint16_t n = 0;
                    if (vesa_csi_params_n) n = vesa_csi_params[0];

                    if (n >= 30 && n <= 37) { // set foreground
                        vfont_fg = VESA_COLORS[n - 30];
                    } else if (n >= 40 && n <= 47) { // set background
                        vfont_bg = VESA_COLORS[n - 40];
                    } else if (n >= 90 && n <= 97) { // set bright foreground
                        vfont_fg = VESA_COLORS[n - 90 + 8];
                    } else if (n >= 100 && n <= 107) { // set bright background
                        vfont_bg = VESA_COLORS[n - 100 + 8];
                    } else {
                        switch (n) {
                            case 0: // reset
                                vesa_bold = vesa_italic = vesa_underline = false;
                                vfont_fg = RGB32(255, 255, 255);
                                vfont_bg = RGB32(0, 0, 0);
                                break;
                            case 1: // bold
                                vesa_bold = true;
                                break;
                            case 3: // italic
                                vesa_italic = true;
                                break;
                            case 4: // underline
                                vesa_underline = true;
                                break;
                            case 7: { // video invert
                                rgb32_t orig_fg = vfont_fg;
                                vfont_fg = vfont_bg;
                                vfont_bg = orig_fg;
                                break;
                            }
                            case 22: // bold off
                                vesa_bold = false;
                                break;
                            case 23: // italic off
                                vesa_italic = false;
                                break;
                            case 24: // underline off
                                vesa_underline = false;
                                break;
                            case 27: { // video invert off
                                rgb32_t orig_fg = vfont_fg;
                                vfont_fg = vfont_bg;
                                vfont_bg = orig_fg;
                                break;
                            }
                            case 38: { // set foreground
                                if (vesa_csi_params[1] == 5)
                                    vfont_fg = col256_to_rgb32(vesa_csi_params[2]);
                                else if (vesa_csi_params[1] == 2)
                                    vfont_fg = RGB32(vesa_csi_params[2], vesa_csi_params[3], vesa_csi_params[4]);
                                break;
                            }
                            case 39: { // default foreground
                                vfont_fg = RGB32(255, 255, 255);
                                break;
                            }
                            case 48: { // set background
                                if (vesa_csi_params[1] == 5)
                                    vfont_bg = col256_to_rgb32(vesa_csi_params[2]);
                                else if (vesa_csi_params[1] == 2)
                                    vfont_bg = RGB32(vesa_csi_params[2], vesa_csi_params[3], vesa_csi_params[4]);
                                break;
                            }
                            case 49: { // default background
                                vfont_bg = RGB32(0, 0, 0);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    break;
                }
                default: { // Unsupported / undefined
                    break;
                }
            }

            memset(vesa_csi_parambuf, 0, sizeof(vesa_csi_parambuf));
            vesa_csi_params_n = 0;

        } else {} // Unsupported / malformed are ignored

    } else {

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
                vesa_drawrect(vesa_x * vfont->width, vesa_y * vfont->height, vfont->width, vfont->height, vfont_bg);
                break;
            case '\033': // \e
                vesa_esc = true;
                break;
            case '[':
                if (vesa_esc) {
                    vesa_esc = false;
                    vesa_csi = true;
                    break;
                }
                /* fall through */
            default:
                psf2_putvesa(vfont, c, vesa_x * vfont->width, vesa_y * vfont->height);
                vesa_x++;
                break;
        }
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
