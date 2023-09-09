#pragma once

#include "types.h"
#include "gdt.h"
#include "idt.h"
#include "schedule.h"

#define APIC_BASE 0xFEE00000

#define MAX_CORES 16

#define MADT_TYPE_LAPIC 0
#define MADT_TYPE_IOAPIC 1
#define MADT_TYPE_ISO 2
#define MADT_TYPE_NMI 4
#define MADT_TYPE_LAPIC_OVERRIDE 5

#define IOAPIC_REG_ID 0x00
#define IOAPIC_REG_VERSION 0x01
#define IOAPIC_REG_ARB 0x02
#define IOAPIC_REG_REDIR(n) (0x10 + (n) * 2)

#define IOAPIC_REDIR_VECTOR_MASK 0xFF
#define IOAPIC_REDIR_VECTOR(n) ((n) & IOAPIC_REDIR_VECTOR_MASK)
#define IOAPIC_REDIR_DMODE_MASK   (7 << 8)
#define IOAPIC_REDIR_DMODE_FIXED  (0 << 8)
#define IOAPIC_REDIR_DMODE_LOWEST (1 << 8)
#define IOAPIC_REDIR_DMODE_SMI    (2 << 8)
#define IOAPIC_REDIR_DMODE_NMI    (4 << 8)
#define IOAPIC_REDIR_DMODE_INIT   (5 << 8)
#define IOAPIC_REDIR_DMODE_EXTINT (7 << 8)
#define IOAPIC_REDIR_DESTMODE_PHYSICAL (0 << 11)
#define IOAPIC_REDIR_DESTMODE_LOGICAL  (1 << 11)
#define IOAPIC_REDIR_DELIVERY_STANDBY (0 << 12)
#define IOAPIC_REDIR_DELIVERY_PENDING (1 << 12)
#define IOAPIC_REDIR_PINPOL_LOW  (0 << 13)
#define IOAPIC_REDIR_PINPOL_HIGH (1 << 13)
#define IOAPIC_REDIR_TRIGMODE_EDGE  (0 << 15)
#define IOAPIC_REDIR_TRIGMODE_LEVEL (1 << 15)
#define IOAPIC_REDIR_MASK   (1 << 16)
#define IOAPIC_REDIR_UNMASK (0 << 16)
#define IOAPIC_REDIR_DEST_MASK ((uint64_t)0xFF << 56)
#define IOAPIC_REDIR_DEST(n) (((uint64_t)n) << 56)

#define IOAPIC_OFFSET 0x20

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
    bool bsp;

    // We need a core-GDT because each core uses a different IST
    gdtr_t gdtr;
    gdt_entry_t * gdt;
    tss_t * tss;

    uint64_t * pml4t;

    process_t * current_process;
} cpu_coreinfo_t;

// Standard ISA IRQs
enum {
    IRQ_PIT = 0,
    IRQ_KBD,
    IRQ_CASCADE,
    IRQ_COM2,
    IRQ_COM1,
    IRQ_LPT2,
    IRQ_DSP = 5,
    IRQ_FLOPPY,
    IRQ_LPT1,
    IRQ_CMOS,
    IRQ_FREE1,
    IRQ_FREE2,
    IRQ_FREE3,
    IRQ_MOUSE,
    IRQ_FPU,
    IRQ_ATA_P,
    IRQ_ATA_S,
};

extern cpu_coreinfo_t * coreinfos;
extern size_t cpu_cores;

void init_apic();

uint32_t ioapic_read(void * ioapic_base, uint32_t reg);
void ioapic_write(void * ioapic_base, uint32_t reg, uint32_t val);

void ioapic_mask(uint8_t irq, bool mask);
void apic_ipi(uint8_t apic_id, uint8_t vector, uint64_t flags);

void apic_write(uint32_t reg, uint32_t val);
uint32_t apic_read(uint32_t reg);

inline __attribute__((always_inline)) uint8_t get_lapic_id() {
    uint8_t id;
    asm volatile ("mov $1, %%eax; cpuid; shrl $24, %%ebx;" : "=b"(id) : : "eax", "ecx", "edx");
    return id;
}
