#pragma once

#include "types.h"

typedef struct gdtr_t {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) gdtr_t;

typedef struct gdt_entry_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t limit_high : 4;
    uint8_t flags : 4;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct gdt_sysentry_t {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle_low;
    uint8_t access;
    uint8_t limit_high : 4;
    uint8_t flags : 4;
    uint8_t base_middle_high;
    uint32_t base_high;
    uint32_t reserved;
}  __attribute__((packed)) gdt_sysentry_t;

typedef struct tss_t {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

#define GDT_ENTRIES 64
#define GDT_BASE 0x0 // TODO: Put this somewhere else, this overwrites some data from the BIOS we might want
extern gdt_entry_t * kgdt;
extern gdtr_t kgdtr;
extern tss_t ktss;

// Access
#define GDT_P (1 << 7)      // Present bit
#define GDT_DPL (3 << 5)    // Privilege level
#define GDT_S (1 << 4)      // Type
#define GDT_E (1 << 3)      // Executable
#define GDT_DC (1 << 2)     // Direction/conforming
#define GDT_RW (1 << 1)     // Readable/writeable
#define GDT_A  (1 << 0)     // Access (unused)

// Flags
#define GDT_G  (1 << 3)     // Granularity
#define GDT_DB (1 << 2)     // Size flag
#define GDT_L  (1 << 1)     // Long mode code

// System access
#define GDT_TSS16 (0x1)     // 16-bit TSS
#define GDT_TSS32 (0x9)     // 32-bit TSS
#define GDT_TSS64 (0x9)     // 64-bit TSS
#define GDT_LDT   (0x2)     // LDT

void fill_gdt_entry(gdt_entry_t * entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
void fill_gdt_sysentry(gdt_sysentry_t * entry, uint64_t base, uint32_t limit, uint8_t access, uint8_t flags);

void init_kgdt();
