#pragma once

#include "types.h"

// The lower 32 bytes are the normal bitmap,
// the higher 32 are the special keys
// (256 keys each, altough a lot goes unused
// These are for SCAN CODES, not ASCII
extern uint8_t * kbd_bitmap;

// A simple handle to access the last pressed key as ASCII
extern char kbd_last_ascii;

void kbd_setkey(uint16_t key, bool status);
bool kbd_getkey(uint16_t key);
char scancode_to_ascii(uint8_t s);
char scancode_shift_to_ascii(uint8_t s);

char kbd_get_last_ascii();

#define KBD_DEFAULT_TRANSLATOR scs1_usqwerty
// First column is normal, second is with shift
static uint8_t scs1_usqwerty[] = {
    0x00, 0x00,
	0x1B, 0x1B,	/* esc (0x01)  */
	'1', '!',
	'2', '@',
	'3', '#',
	'4', '$',
	'5', '%',
	'6', '^',
	'7', '&',
	'8', '*',
	'9', '(',
	'0', ')',
	'-', '_',
	'=', '+',
	'\b', '\b',	/* backspace */
	'\t', '\t',	/* tab */
	'q', 'Q',
	'w', 'W',
	'e', 'E',
	'r', 'R',
	't', 'T',
	'y', 'Y',
	'u', 'U',
	'i', 'I',
	'o', 'O',
	'p', 'P',
	'[', '{',
	']', '}',
	'\n', '\n',	/* enter */
	0x00, 0x00,	/* ctrl */
	'a', 'A',
	's', 'S',
	'd', 'D',
	'f', 'F',
	'g', 'G',
	'h', 'H',
	'j', 'J',
	'k', 'K',
	'l', 'L',
	';', ':',
	'\'', '\"',	/* '" */
	'`', '~',   /* `~ */
	0x00, 0x00,	/* Lshift  (0x2a)  */
	'\\', '|',
	'z', 'Z',
	'x', 'X',
	'c', 'C',
	'v', 'V',
	'b', 'B',
	'n', 'N',
	'm', 'M',
	',', '<',	/* ,< */
	'.', '>',	/* .> */
	'/', '?',	/* /? */
	0x00, 0x00,	/* Rshift  (0x36)  */
	0x00, 0x00,	/* (0x37)  */
	0x00, 0x00,	/* Alt (0x38) */
	' ', ' ',   /* space   */
};

// Special keys on the NORMAL key bitmap
#define SCS1_CTRL   0x1D
#define SCS1_LSHIFT 0x2A
#define SCS1_RSHIFT 0x36
#define SCS1_ALT    0x38
#define SCS1_CAPS   0x3A
#define SCS1_F1     0x3B
#define SCS1_F2     0x3C
#define SCS1_F3     0x3D
#define SCS1_F4     0x3E
#define SCS1_F5     0x3F
#define SCS1_F6     0x40
#define SCS1_F7     0x41
#define SCS1_F8     0x42
#define SCS1_F9     0x43
#define SCS1_F10    0x44
#define SCS1_F11    0x57
#define SCS1_F12    0x58
