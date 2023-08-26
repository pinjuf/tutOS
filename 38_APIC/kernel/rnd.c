#include "rnd.h"
#include "util.h"

uint64_t rnd_seed = 0;

void srand(uint64_t seed) {
    rnd_seed = seed;
}

uint32_t rand() {
    // Linear congruential generator
    const uint64_t a = 6364136223846793005ULL;
    const uint64_t c = 1442695040888963407ULL;
    // m = 2^64 = RAND_MAX + 1

    rnd_seed = rnd_seed * a + c;
    return rnd_seed >> 32;
}
