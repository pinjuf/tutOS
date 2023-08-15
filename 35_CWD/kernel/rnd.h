#pragma once

#include "types.h"

extern uint64_t rnd_state;

void srand(uint64_t seed);
uint64_t rand();

#define RAND_MAX UINT64_MAX
