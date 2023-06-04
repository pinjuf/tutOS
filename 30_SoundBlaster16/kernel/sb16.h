#pragma once

#include "types.h"

#define DSP_BUF ((void*)0x200000)
#define DSP_SIZE 0x10000
#define DSP_SAMPLING_RATE

#define DSP_MIXER      0x224
#define DSP_MIXER_DATA 0x225
#define DSP_RESET      0x226
#define DSP_READ       0x22A
#define DSP_WRITE      0x22C
#define DSP_STATUS     0x22E
#define DSP_IRQACK8    0x22E
#define DSP_IRQACK16   0x22F

#define DSP_TO_TIMECONSTANT(sampling_rate) (256 - (1000000 / (sampling_rate)))

// DSP Tranfer modes
#define DSP_TRA16  0xB0
#define DSP_TRA8   0xC0
#define DSP_RECORD (1<<3)
#define DSP_FIFO   (1<<1)

// DSP Sound data types
#define DSP_STEREO (1<<5)
#define DSP_SIGNED (1<<4)

void init_sb16();
void sb16_volume(uint8_t volume);
void sb16_play();

// Generic structure for what we are currently playing
typedef struct sb16_player_t {
    void * data;
    void * current;
    size_t size; // Actual size, not the count
    uint16_t sampling_rate;
    uint8_t volume;
    uint8_t _16bit : 1;
    uint8_t _sign  : 1;
} sb16_player_t;

extern sb16_player_t * sb16_player;
