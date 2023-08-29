#pragma once

// Everything auxiliary processor

#define AP_STARTUP 0xA000
#define AP_STACKSZ 0x1000

extern volatile void * ap_stack;

void init_ap(void);
