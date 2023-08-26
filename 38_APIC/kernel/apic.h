#pragma once

#include "types.h"

#define APIC_BASE 0xFEE00000

#define MAX_CORES 16

#define MADT_TYPE_LAPIC 0
#define MADT_TYPE_IOAPIC 1
#define MADT_TYPE_ISO 2
#define MADT_TYPE_NMI 4
#define MADT_TYPE_LAPIC_OVERRIDE 5

typedef struct madt_lapic_t {
    uint8_t type;
    uint8_t length;

    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) madt_lapic_t;

typedef struct madt_ioapic_t {
    uint8_t type;
    uint8_t length;

    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) madt_ioapic_t;

typedef struct madt_iso_t {
    uint8_t type;
    uint8_t length;

    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) madt_iso_t;

typedef struct cpu_coreinfo_t {
    uint8_t processor_id;
    uint8_t apic_id;
    bool available;
    bool bsp;
} cpu_coreinfo_t;

extern void * ioapic_base;
extern cpu_coreinfo_t * coreinfos;
extern size_t cpu_cores;

void init_apic();

uint32_t ioapic_read(uint32_t reg);
void ioapic_write(uint32_t reg, uint32_t val);
