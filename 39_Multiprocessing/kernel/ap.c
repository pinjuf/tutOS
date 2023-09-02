#include "ap.h"
#include "apic.h"
#include "main.h"
#include "util.h"
#include "mm.h"

void init_ap() {
    bpob->ap_entry = ap_entry;

    // Shamelessly stolen from the OSDev wiki
    for (size_t i = 0; i < cpu_cores; i++) {
        cpu_coreinfo_t * core = &coreinfos[i];
        if (core->bsp || !core->available) continue; // Whatever is executing this very code IS the BSP

        bpob->ap_stack = (void*)((size_t)kmalloc(AP_STACKSZ) + AP_STACKSZ);

        uint8_t apic_id = core->apic_id;

        apic_write(0x280, 0); // Clear errors
        apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (apic_id << 24)); // Select APIC ID
        apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0xC500); // INIT IPI assert
        while (apic_read(0x300) & (1 << 12)); // Wait for delivery
        apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (apic_id << 24)); // Select APIC ID
        apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0x8500); // INIT IPI deassert
        while (apic_read(0x300) & (1 << 12)) {asm volatile ("pause" : : : "memory");}; // Wait for delivery
        usleep(20000); // Let the AP wake up (10 ms recommended, we'll do 20 ms)

        // Send 2 SIPIs
        for(int j = 0; j < 2; j++) { // TODO: Repeat until AP is awake (sth. like bpob->awake_aps++ by the AP)
            apic_write(0x280, 0); // Clear errors
            apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (apic_id << 24)); // Select APIC ID
            apic_write(0x300, (apic_read(0x300) & 0xFFF0F800) | 0x600 | (AP_TRAMPOLINE / PAGE_SIZE)); // INIT IPI
            usleep(200); // wait 200 usec
            while (apic_read(0x300) & (1 << 12)) {asm volatile ("pause" : : : "memory");}; // Wait for delivery
        }

        while (bpob->ap_stack); // AP takes the stack and sets that to NULL when it's ready
    }
}

void ap_entry() {
    // Application processors enter here after the trampoline
    cpu_coreinfo_t * core = NULL;
    for (size_t i = 0; i < cpu_cores; i++) {
        if (coreinfos[i].apic_id == apic_read(0x20) >> 24) {
            core = &coreinfos[i];
            break;
        }
    }

    init_apgdt(core);

    // We can use the IDT of the BSP (only the BSP gets IRQs)
    asm volatile ("lidt %0" : : "m" (kidtr));

    while (1);
}
