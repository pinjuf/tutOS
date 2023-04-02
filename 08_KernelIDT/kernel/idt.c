#include "idt.h"
#include "pic.h"

idtr_t kidtr;
idt_entry_t * kidt = (idt_entry_t *) IDT_BASE;

extern void * isr_stub_table[];
extern void isr_default_stub();
extern void isr_irq0_stub();

void fill_idt_desc(idt_entry_t * entry, void * isr, uint8_t flags, uint8_t selector) {
    entry->offset_low = (uint64_t)isr & 0xFFFF;
    entry->selector = selector;
    entry->ist = 0;
    entry->attr = flags;
    entry->offset_middle = ((uint64_t)isr >> 16) & 0xFFFF;
    entry->offset_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    entry->reserved = 0;
}

void init_idt(void) {
    kidtr.offset = (uint64_t)kidt;
    kidtr.size   = sizeof(idt_entry_t) * IDT_ENTRIES - 1;

    for (uint16_t i = 0; i < IDT_ENTRIES; i++) {
        if (i < 32) {
            fill_idt_desc(&kidt[i], isr_stub_table[i], IDT_P | IDT_TRAP, 0x08);
        } else {
            fill_idt_desc(&kidt[i], isr_default_stub, IDT_P | IDT_INT, 0x08);

        }
    }

    fill_idt_desc(&kidt[PIC_OFFSET+PIC_PIT], isr_irq0_stub, IDT_P | IDT_INT, 0x08);

    asm volatile ("lidt %0" : : "m" (kidtr));
}
