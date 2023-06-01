#pragma once

#define PIT0_FREQ 2000

#include "vesa.h"

// Boot Pass-On Block (custom)
typedef struct bpob_t {
    vbe_info_t vbe_info;
    vbe_mode_info_t vbe_mode_info;
} __attribute__((packed)) bpob_t;

#define BPOB_ADDR 0x7E00
extern bpob_t * bpob;
