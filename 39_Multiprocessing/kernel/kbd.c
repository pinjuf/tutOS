#include "kbd.h"

#include "util.h"

bool mouse_left   = false;
bool mouse_middle = false;
bool mouse_right  = false;
uint8_t mouse_scroll = PS2_MOUSE_SCRL_NONE;

float mouse_x = 0;
float mouse_y = 0;

size_t mouse_xlim = 800; // These values get changed on VESA init
size_t mouse_ylim = 600;

uint8_t * kbd_bitmap;

volatile char kbd_last_ascii;
volatile uint16_t kbd_last_scancode;

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

char scancode_to_ascii(uint8_t s) {
    if (s*2 >= sizeof(KBD_DEFAULT_TRANSLATOR))
        return 0;
    return KBD_DEFAULT_TRANSLATOR[s * 2];
}

char scancode_shift_to_ascii(uint8_t s) {
    if (s*2 >= sizeof(KBD_DEFAULT_TRANSLATOR))
        return 0;
    return KBD_DEFAULT_TRANSLATOR[s * 2 + 1];
}

// Should it be necessary to get the last key WITHOUT blocking, you can just read kbd_last_ascii
char kbd_get_last_ascii() {
    while (kbd_last_ascii == 0);
    char o = kbd_last_ascii;
    kbd_last_ascii = 0;
    return o;
}

uint16_t kbd_get_last_scancode() {
    while (kbd_last_scancode == 0);
    uint16_t o = kbd_last_scancode;
    kbd_last_scancode = 0;
    return o;
}
