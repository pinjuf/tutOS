#include "ap.h"
#include "apic.h"
#include "main.h"
#include "util.h"
#include "mm.h"

void init_ap() {
    bpob->ap_entry = ap_entry;
    bpob->ap_count = 0;

    // Shamelessly stolen from the OSDev wiki
    for (size_t i = 0; i < cpu_cores; i++) {
        kprintf("Waking up AP #%d\n", i);

        cpu_coreinfo_t * core = &coreinfos[i];
        if (core->bsp || !core->available) continue; // Whatever is executing this very code IS the BSP

        bpob->ap_stack = (void*)((size_t)alloc_pages(AP_STACKSZ / PAGE_SIZE) + AP_STACKSZ);

        uint8_t apic_id = core->apic_id;

        apic_write(0x280, 0); // Clear errors
        apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (apic_id << 24)); // Select APIC ID
        apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0xC500); // INIT IPI assert
        while (apic_read(0x300) & (1 << 12)); // Wait for delivery
        apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (apic_id << 24)); // Select APIC ID
        apic_write(0x300, (apic_read(0x300) & 0xFFF00000) | 0x8500); // INIT IPI deassert
        while (apic_read(0x300) & (1 << 12)) {asm volatile ("pause" : : : "memory");}; // Wait for delivery
        usleep(20000); // Let the AP wake up (10 ms recommended, we'll do 20 ms)

        uint8_t original_ap_count = bpob->ap_count;

        // Send SIPIs until the AP wakes up
        while (original_ap_count == bpob->ap_count) {
            apic_write(0x280, 0); // Clear errors
            apic_write(0x310, (apic_read(0x310) & 0x00FFFFFF) | (apic_id << 24)); // Select APIC ID
            apic_write(0x300, (apic_read(0x300) & 0xFFF0F800) | 0x600 | (AP_TRAMPOLINE / PAGE_SIZE)); // INIT IPI
            usleep(200); // wait 200 usec
            while (apic_read(0x300) & (1 << 12)) {asm volatile ("pause" : : : "memory");}; // Wait for delivery
        }

        kprintf("AP #%d is online!\n", i);
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

    char * test = kmalloc(128);
    strcpy(test, "Hello from AP #");
    itoa(core->apic_id, test + strlen(test), 10);
    kprintf("%s\n", test);

    // We can use the IDT of the BSP (only the BSP gets IRQs)
    asm volatile ("lidt %0" : : "m" (kidtr));

    // Set up our own page tables
    void * pml4 = calloc_pages(1);

    // Map the kernel from 0x0 to 0x400000
    _mmap_page_2mb(pml4, (void*)NULL, (void*)NULL, PAGE_PRESENT | PAGE_RW);
    _mmap_page_2mb(pml4, (void*)0x200000, (void*)0x200000, PAGE_PRESENT | PAGE_RW);

    // Map the heap
    for (size_t i = 0; i < HEAP_PTS; i++) {
        _mmap_page_2mb(pml4, (void*)(HEAP_VIRT + i * PAGE_SIZE * PAGE_ENTRIES), (void*)(HEAP_PHYS + i * PAGE_SIZE * PAGE_ENTRIES), HEAP_FLAGS);
    }

    // Map the APIC
    _mmap_page(pml4, (void*)APIC_BASE, (void*)APIC_BASE, PAGE_PRESENT | PAGE_RW);

    void * x = get_pml4t();
    memcpy(x, pml4, PAGE_SIZE);

    // Abandon the BSP page tables
    asm volatile ("mov %0, %%cr3" : : "a" (virt_to_phys(pml4)));

    kprintf("AP #%d is ready!\n", core->apic_id);

    while (1);
}
