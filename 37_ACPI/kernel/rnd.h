#pragma once

#include "types.h"

extern uint64_t rnd_state;

void srand(uint64_t seed);
uint32_t rand();

#define RAND_MAX UINT32_MAX
