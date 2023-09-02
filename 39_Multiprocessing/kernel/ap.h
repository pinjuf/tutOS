#pragma once

// Application processor / Symmetric multiprocessing stuff

#define AP_TRAMPOLINE 0xA000
#define AP_STACKSZ 0x1000

void init_ap();
void ap_entry();
