#include "ap.h"
#include "apic.h"
#include "util.h"
#include "mm.h"

volatile void * ap_stack = NULL;

void init_ap(void) {
    // Start all APs

    for (uint8_t i = 0; i < cpu_cores; i++) {
        cpu_coreinfo_t * core = &coreinfos[i];
        if (core->bsp) continue; // Don't start ourselves

        apic_clear_errors();
        apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (i << 24));
        apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0xC500);
        apic_wait_ipi();

        apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (i << 24));
        apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0x8500);
        apic_wait_ipi();

        usleep(10000);
        for (uint8_t j = 0; j < 2; j++) {
            apic_clear_errors();
            apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (i << 24));
            apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0x600 | AP_STARTUP/PAGE_SIZE);
            usleep(200);
            apic_wait_ipi();
        }

        kprintf("asdf");
        while (1) {
            hexdump((void*)0x9C00, 2);
        }
    }
}
