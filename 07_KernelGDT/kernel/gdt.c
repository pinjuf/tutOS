#include "gdt.h"
#include "util.h"

gdt_entry_t kgdt[GDT_ENTRIES];
gdtr_t kgdtr;

void fill_gdt_entry(gdt_entry_t * entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    entry->limit_low = limit & 0xFFFF;
    entry->limit_high = (limit >> 16) & 0xF;

    entry->base_low    = base & 0xFFFF;
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high   = (base >> 24) & 0xFF;

    entry->access = access;
    entry->flags  = flags;
}

void fill_gdt_sysentry(gdt_sysentry_t * entry, uint64_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    fill_gdt_entry((gdt_entry_t *) entry, base & 0xFFFFFFFF, limit, access, flags);
    entry->base_high = (base >> 32) & 0xFFFFFFFF;
    entry->reserved  = 0;
}

void init_kgdt() {
    kgdtr.size = sizeof(kgdt) - 1;
    kgdtr.offset  = (uint64_t)&kgdt;

    memset(kgdt, 0, sizeof(kgdt));

    fill_gdt_entry(&kgdt[1],
            0,
            0xFFFFFFFF,
            GDT_P | GDT_S | GDT_E | GDT_RW,
            GDT_G | GDT_L
    ); // kcode

    asm volatile ("lgdt %0" : : "m"(kgdtr));

    // Long mode essentially only uses CS
    asm volatile ("\
            xor %ax, %ax; \
            movw %ax, %ds; \
            movw %ax, %es; \
            movw %ax, %fs; \
            movw %ax, %gs; \
            movw %ax, %ss; \
    ");

    // Far return
    asm volatile ("\
        pushq $0x08; \
        pushq $next; \
        retfq; \
        next: \
    ");
}
