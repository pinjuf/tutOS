#pragma once

#include "types.h"

#define IDT_ENTRIES 256
#define IDT_BASE 0x400

typedef struct idtr_t {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed)) idtr_t;

typedef struct idt_entry_t {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

extern idtr_t kidtr;
extern idt_entry_t * kitd;

void fill_idt_desc(idt_entry_t * entry, void * isr, uint8_t flags, uint8_t selector);
void init_idt(void);

#define IDT_P (1 << 7)      // Present
#define IDT_DPL (0b11 << 5) // Privilege level
#define IDT_INT (0xE)       // Interrupt gate
#define IDT_TRAP (0xF)      // Trap gate
