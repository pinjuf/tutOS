#include "gdt.h"
#include "util.h"

gdt_entry_t * kgdt = (gdt_entry_t *) GDT_BASE;
gdtr_t kgdtr;
tss_t ktss;

void fill_gdt_entry(gdt_entry_t * entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    entry->limit_low  = limit & 0xFFFF;
    entry->limit_high = (limit >> 16) & 0xF;

    entry->base_low    = base & 0xFFFF;
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high   = (base >> 24) & 0xFF;

    entry->access = access;
    entry->flags  = flags & 0x0F;
}

void fill_gdt_sysentry(gdt_sysentry_t * entry, uint64_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    fill_gdt_entry((gdt_entry_t *) entry, base & 0xFFFFFFFF, limit, access, flags);
    entry->base_high = (base >> 32) & 0xFFFFFFFF;
    entry->reserved  = 0;
}

void init_kgdt() {
    kgdtr.size = sizeof(gdt_entry_t) * GDT_ENTRIES - 1;
    kgdtr.offset  = (uint64_t)kgdt;

    memset(kgdt, 0, sizeof(gdt_entry_t) * GDT_ENTRIES);

    memset(&ktss, 0, sizeof(ktss));

    for (size_t i = 0; i < 3; i++)
        ktss.rsp[i] = 0x110000;
    for (size_t i = 0; i < 7; i++)
        ktss.ist[i] = 0x110000;

    fill_gdt_entry(&kgdt[1],
            0,
            0,
            GDT_P | GDT_S | GDT_E | GDT_RW,
            GDT_L
    ); // kcode

    fill_gdt_entry(&kgdt[2],
            0,
            0,
            GDT_P | GDT_S | GDT_DC | GDT_RW,
            0
    ); // kstack

    fill_gdt_sysentry((gdt_sysentry_t*)&kgdt[3],
            (uint64_t)&ktss,
            sizeof(tss_t)-1,
            GDT_P | GDT_TSS64,
            0
    ); // ktss

    fill_gdt_entry(&kgdt[5],
            0,
            0,
            GDT_P | GDT_DPL | GDT_S | GDT_DC | GDT_RW,
            0
    ); // ustack

    fill_gdt_entry(&kgdt[6],
            0,
            0,
            GDT_P | GDT_DPL | GDT_S | GDT_E | GDT_RW,
            GDT_L
    ); // ucode

    asm volatile ("lgdt %0" : : "m"(kgdtr));
    asm volatile ("ltr %%ax" : : "a"(3 * 0x8));

    // Long mode essentially only uses CS (and sometimes SS)
    asm volatile ("\
            xor %ax, %ax; \
            movw %ax, %ds; \
            movw %ax, %es; \
            movw %ax, %fs; \
            movw %ax, %gs; \
            mov $0x10, %ax; \
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
