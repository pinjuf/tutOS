#pragma once

#define PIT0_FREQ 2000

#include "vesa.h"

// Boot Pass-On Block (custom)
typedef struct bpob_t {
    vbe_info_t vbe_info;
    vbe_mode_info_t vbe_mode_info;
    volatile uint16_t ap_hello;
} __attribute__((packed)) bpob_t;

#define MAP(x, input_start, input_end, output_start, output_end) (x - input_start) / (input_end - input_start) * (output_end - output_start) + output_start
#define BPOB_ADDR 0x7E00
extern bpob_t * bpob;

extern size_t cpu_tps;

#define KERN_PID 0 // Special PID, indicates low kernel

#define INIT_PID  1
#define INIT_PATH "/bin/init"