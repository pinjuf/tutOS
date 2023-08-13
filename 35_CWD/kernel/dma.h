#pragma once

// ISA 8237 DMA

#include "types.h"

// Slave has 8-bit channels 0-3 (0 is unusable)
// Master has 16-bit channels 4-7 (4 is unusable)

// Start address registers
static const uint16_t DMA_ADDRESS[] = {
    0x00,
    0x02,
    0x04,
    0x06,
    0xC0,
    0xC4,
    0xC8,
    0xCC,
};

static const uint16_t DMA_COUNT[] = {
    0x01,
    0x03,
    0x05,
    0x06,
    0xC2,
    0xC6,
    0xCA,
    0xCE,
};

static const uint16_t DMA_PAGE[] = {
    0x87,
    0x83,
    0x81,
    0x82,
    0x8F,
    0x8B,
    0x89,
    0x8A,
};

static const uint16_t DMA_STATUS[] = {
    0x08,
    0xD0,
};

static const uint16_t DMA_COMMAND[] = {
    0x08,
    0xD0,
};

static const uint16_t DMA_REQUEST[] = {
    0x09,
    0xD2,
};

static const uint16_t DMA_SINGLE_MASK[] = {
    0x0A,
    0xD4,
};

static const uint16_t DMA_MODE[] = {
    0x0B,
    0xD6,
};

static const uint16_t DMA_FLIP_FLOP[] = {
    0x0C,
    0xD8,
};

static const uint16_t DMA_INTERMEDIATE[] = {
    0x0D,
    0xDA,
};

static const uint16_t DMA_MASTER_RESET[] = {
    0x0D,
    0xDA,
};

static const uint16_t DMA_MASK_RESET[] = {
    0x0E,
    0xDC,
};

static const uint16_t DMA_MASK[] = {
    0x0F,
    0xDE,
};

// Transfer types for mode registers
#define DMA_TRA_SELFTEST 0
#define DMA_TRA_WRITE    4
#define DMA_TRA_READ     8
// Reset address and count registers after finishing
#define DMA_AUTOINIT  0x10
// Read in reverse order
#define DMA_DOWNWARDS 0x20
// Transfer modes
#define DMA_ONDEMAND 0x00
#define DMA_SINGLE   0x40
#define DMA_BLOCK    0x80
#define DMA_CASCADAE 0xC0

int dma_chan_init(uint8_t chan, void * addr, uint32_t count);
int dma_chan_mode(uint8_t chan, uint8_t mode);
