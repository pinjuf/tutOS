#pragma once

#include "types.h"

typedef struct wavhdr_t {
    uint8_t RIFF[4];
    uint32_t ChunkSize;
    uint8_t WAVE[4];

    uint8_t FMT[4];
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;

    uint8_t DATA[4];
    uint32_t Subchunk2Size;
} __attribute__((packed)) wavhdr_t;

#define WAVE_FORMAT_PCM   0x1
#define WAVE_FORMAT_IEEE  0x3
#define WAVE_FORMAT_ALAW  0x6
#define WAVE_FORMAT_MULAW 0x7
#define WAVE_FORMAT_EXT   0xFFFE
