#include "kbd.h"

uint8_t * kbd_bitmap;

void kbd_setkey(uint16_t key, bool status) {
    uint16_t i = key/8;
    uint8_t  j = key%8;

    if (status) {
        kbd_bitmap[i] |= 1<<j;
    } else {
        kbd_bitmap[i] &= ~(1<<j);
    }
}

bool kbd_getkey(uint16_t key) {
    uint16_t i = key/8;
    uint8_t  j = key%8;

    return (kbd_bitmap[i] & (1<<j)) != 0;
}
