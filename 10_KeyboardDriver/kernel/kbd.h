#pragma once

#include "types.h"

// The lower 32 bytes are the normal bitmap,
// the higher 32 are the special keys
// (256 keys each, altough a lot goes unused
// These are for SCAN CODES, not ASCII
extern uint8_t * kbd_bitmap;

void kbd_setkey(uint16_t key, bool status);
bool kbd_getkey(uint16_t key);
