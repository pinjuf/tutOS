#pragma once

#include "apic.h"

// Application processor / Symmetric multiprocessing stuff

#define AP_TRAMPOLINE 0xA000
#define AP_STACKSZ 0x1000

void init_ap();
void ap_entry();

cpu_coreinfo_t * get_core();
