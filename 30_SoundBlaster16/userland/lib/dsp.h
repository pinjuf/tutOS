#include "types.h"

typedef struct sb16_player_t {
    void * data;
    size_t current;
    size_t size; // Actual size, not the count
    uint16_t sampling_rate;
    uint8_t volume;
    bool _16bit;
    bool sign;
    bool stereo;
    bool playing;
} __attribute__((packed)) sb16_player_t;
