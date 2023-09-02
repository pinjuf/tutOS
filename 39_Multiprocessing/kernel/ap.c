#include "ap.h"
#include "apic.h"
#include "main.h"
#include "util.h"

void init_ap() {
    bpob->ap_hello = 0;

    // Shamelessly stolen from the OSDev wiki
    for (size_t i = 0; i < cpu_cores; i++) {
        cpu_coreinfo_t * core = &coreinfos[i];
        if (core->bsp) continue; // Whatever is executing this very code IS the BSP

        uint8_t apic_id = core->apic_id;

        *((volatile uint32_t*)(APIC_BASE + 0x280)) = 0;                                                                             // clear APIC errors
        *((volatile uint32_t*)(APIC_BASE + 0x310)) = (*((volatile uint32_t*)(APIC_BASE + 0x310)) & 0x00ffffff) | (apic_id << 24);         // select AP
        *((volatile uint32_t*)(APIC_BASE + 0x300)) = (*((volatile uint32_t*)(APIC_BASE + 0x300)) & 0xfff00000) | 0x00C500;          // trigger INIT IPI
        do { __asm__ __volatile__ ("pause" : : : "memory"); }while(*((volatile uint32_t*)(APIC_BASE + 0x300)) & (1 << 12));         // wait for delivery
        *((volatile uint32_t*)(APIC_BASE + 0x310)) = (*((volatile uint32_t*)(APIC_BASE + 0x310)) & 0x00ffffff) | (apic_id << 24);         // select AP
        *((volatile uint32_t*)(APIC_BASE + 0x300)) = (*((volatile uint32_t*)(APIC_BASE + 0x300)) & 0xfff00000) | 0x008500;          // deassert
        do { __asm__ __volatile__ ("pause" : : : "memory"); }while(*((volatile uint32_t*)(APIC_BASE + 0x300)) & (1 << 12));         // wait for delivery
        usleep(10000);                                                                                                                 // wait 10 msec
        // send STARTUP IPI (twice)
        for(int j = 0; j < 2; j++) {
            *((volatile uint32_t*)(APIC_BASE + 0x280)) = 0;                                                                     // clear APIC errors
            *((volatile uint32_t*)(APIC_BASE + 0x310)) = (*((volatile uint32_t*)(APIC_BASE + 0x310)) & 0x00ffffff) | (apic_id << 24); // select AP
            *((volatile uint32_t*)(APIC_BASE + 0x300)) = (*((volatile uint32_t*)(APIC_BASE + 0x300)) & 0xfff0f800) | 0x00060A;  // trigger STARTUP IPI for 0800:0000
            usleep(200);                                                                                                        // wait 200 usec
            do { __asm__ __volatile__ ("pause" : : : "memory"); }while(*((volatile uint32_t*)(APIC_BASE + 0x300)) & (apic_id << 12)); // wait for delivery
        }
    }

    kprintf("Started APs: %hu\n", bpob->ap_hello);
}
