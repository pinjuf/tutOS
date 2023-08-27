#include "apic.h"
#include "util.h"
#include "acpi.h"
#include "mm.h"
#include "pic.h"

cpu_coreinfo_t * coreinfos = NULL;
size_t cpu_cores = 0;

uint32_t ioapic_read(void * ioapic_base, uint32_t reg) {
    uint32_t volatile * ioapic = (uint32_t volatile *)ioapic_base;
    ioapic[0] = reg & 0xFF;
    return ioapic[4];
}

void ioapic_write(void * ioapic_base, uint32_t reg, uint32_t val) {
    uint32_t volatile * ioapic = (uint32_t volatile *)ioapic_base;
    ioapic[0] = reg & 0xFF;
    ioapic[4] = val;
}

uint64_t ioapic_read_redtbl(void * ioapic_base, uint8_t entry) {
    uint64_t out = ioapic_read(ioapic_base, IOAPIC_REG_REDIR(entry));
    out |= (uint64_t)ioapic_read(ioapic_base, IOAPIC_REG_REDIR(entry) + 1) << 32;
    return out;
}

void ioapic_write_redtbl(void * ioapic_base, uint8_t entry, uint64_t val) {
    ioapic_write(ioapic_base, IOAPIC_REG_REDIR(entry), (uint32_t)val);
    ioapic_write(ioapic_base, IOAPIC_REG_REDIR(entry) + 1, (uint32_t)(val >> 32));
}

void init_apic() {
    // Check if APIC is supported
    uint32_t unused = 0;
    uint32_t edx, ebx;
    cpuid(1, unused, &unused, &ebx, &unused, &edx);
    if (!(edx & (1 << 9))) {
        kwarn(__FILE__,__func__,"APIC not supported");
    }

    uint8_t bsp = ebx >> 24;

    // Disable the 8259 PIC
    asm volatile (" \
        mov $0xff, %al; \
        outb %al, $0xa1; \
        outb %al, $0x21; \
    ");

    // 2 main parts: Local APIC and I/O APIC
    // When executing this, we are in the BSP (bootstrap processor)
    
    // Get some info about the cores too
    coreinfos = (void*)kmalloc(sizeof(cpu_coreinfo_t) * MAX_CORES);
    madt_t * madt = (void*)(uint64_t)get_acpi_table("APIC");

    void * curr = (void*)((size_t)madt + sizeof(madt_t));
    while ((size_t)curr < (size_t)madt + madt->sdt.length) {
        uint8_t type = *(uint8_t*)curr;
        uint8_t len = *(uint8_t*)((size_t)curr + 1);

        switch (type) {
            case MADT_TYPE_LAPIC: {
                madt_lapic_t * lapic = curr;

                coreinfos[cpu_cores].processor_id = lapic->acpi_processor_id;
                coreinfos[cpu_cores].apic_id      = lapic->apic_id;
                coreinfos[cpu_cores].available    = lapic->flags & 1 ? true : (lapic->flags & 2 ? true : false);

                if (bsp == lapic->acpi_processor_id) {
                    coreinfos[cpu_cores].bsp = true;
                }

                cpu_cores++;

                break;
            }
            case MADT_TYPE_IOAPIC: {
                madt_ioapic_t * ioapic = curr;

                // Map the I/O APIC
                mmap_page((void*)(uint64_t)ioapic->ioapic_address, (void*)(uint64_t)ioapic->ioapic_address, PAGE_PRESENT | PAGE_RW);

                uint8_t entries =  ioapic_read((void*)(uint64_t)ioapic->ioapic_address, IOAPIC_REG_VERSION) >> 16 & 0xFF;
                entries++; // The number of entries is 1 less than the value in the register
                for (uint8_t i = 0; i < entries; i++) {
                    uint8_t entry = i;

                    uint64_t redtbl = ioapic_read_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry);

                    // Set the interrupt vector to 0x20 + entry
                    redtbl &= ~IOAPIC_REDIR_VECTOR_MASK;
                    redtbl |= IOAPIC_OFFSET + ioapic->global_system_interrupt_base + entry;

                    // Mask the interrupt
                    redtbl |= IOAPIC_REDIR_MASK;

                    // Set the destination field to the BSP
                    redtbl &= ~IOAPIC_REDIR_DEST_MASK;
                    redtbl |= IOAPIC_REDIR_DEST(bsp);

                    ioapic_write_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry, redtbl);

                    redtbl = ioapic_read_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry);
                    kprintf("REDTBL for IRQ %d: %x\n", ioapic->global_system_interrupt_base + entry, redtbl);
                }

                // Look for ISOs (Interrupt Source Overrides) affecting this I/O APIC
                // These are used to remap legacy IRQs to new ones, but are a bit unpredictable.
                // Therefore, we will remap them to the original IRQ numbers.
                // Example: IRQ #0 is commonly remapped to IRQ #2, so we must remap the entry
                // for IRQ #2 to use interrupt vector 0x20 + 0 instead of 0x20 + 2.

                uint8_t * curr2 = (void*)((size_t)madt + sizeof(madt_t));
                while ((size_t)curr2 < (size_t)madt + madt->sdt.length) {
                    uint8_t type2 = *(uint8_t*)curr2;
                    uint8_t len2 = *(uint8_t*)((size_t)curr2 + 1);

                    if (type2 != MADT_TYPE_ISO) {
                        curr2 += len2;
                        continue;
                    }

                    madt_iso_t * iso = (void*)curr2;
                    // Check that the GSI is within the range of this I/O APIC
                    if (iso->global_system_interrupt >= ioapic->global_system_interrupt_base && iso->global_system_interrupt < ioapic->global_system_interrupt_base + entries) {
                        uint8_t entry = iso->global_system_interrupt - ioapic->global_system_interrupt_base;

                        uint64_t redtbl = ioapic_read_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry);

                        // Remap the interrupt vector
                        redtbl &= ~IOAPIC_REDIR_VECTOR_MASK;
                        redtbl |= IOAPIC_OFFSET + iso->irq_source;

                        // Flags: Bit 1 is active low, bit 3 is level triggered
                        if (iso->flags & 2) {
                            redtbl |= IOAPIC_REDIR_PINPOL_LOW;
                        } else {
                            redtbl &= ~IOAPIC_REDIR_PINPOL_LOW;
                        }

                        if (iso->flags & 8) {
                            redtbl |= IOAPIC_REDIR_TRIGMODE_LEVEL;
                        } else {
                            redtbl &= ~IOAPIC_REDIR_TRIGMODE_LEVEL;
                        }

                        ioapic_write_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry, redtbl);
                    }

                    curr2 += len2;
                }

                break;
            }
            default:
                break;
        }

        curr = (void*)((size_t)curr + len);
    }

    // Enable the APIC by setting the APIC enable bit in the spurious interrupt vector register
    mmap_page((void*)APIC_BASE, (void*)APIC_BASE, PAGE_PRESENT | PAGE_RW);
    *(uint32_t*)(APIC_BASE + 0xF0) |= 1 << 8 | 0xFF; // 0xFF is the spurious interrupt vector

    // Unmask used interrupts
    ioapic_mask(PIC_PIT, false); // IRQ #0
    ioapic_mask(PIC_KBD, false); // IRQ #1
    ioapic_mask(PIC_MOUSE, false); // IRQ #1
    ioapic_mask(PIC_DSP, false); // IRQ #1
}

void ioapic_mask(uint8_t irq, bool mask) {
    // We need to find the I/O APIC that handles this IRQ

    madt_t * madt = (void*)(uint64_t)get_acpi_table("APIC");

    // Check for ISOs
    void * curr = (void*)((size_t)madt + sizeof(madt_t));
    while ((size_t)curr < (size_t)madt + madt->sdt.length) {
        uint8_t type = *(uint8_t*)curr;
        uint8_t len = *(uint8_t*)((size_t)curr + 1);

        if (type != MADT_TYPE_ISO) {
            curr = (void*)((size_t)curr + len);
            continue;
        }

        madt_iso_t * iso = curr;

        if (iso->irq_source == irq) {
            irq = iso->global_system_interrupt;
            break;
        }

        curr = (void*)((size_t)curr + len);
    }

    curr = (void*)((size_t)madt + sizeof(madt_t));
    while ((size_t)curr < (size_t)madt + madt->sdt.length) {
        uint8_t type = *(uint8_t*)curr;
        uint8_t len = *(uint8_t*)((size_t)curr + 1);

        if (type != MADT_TYPE_IOAPIC) {
            curr = (void*)((size_t)curr + len);
            continue;
        }

        madt_ioapic_t * ioapic = curr;

        uint8_t entries = ioapic_read((void*)(uint64_t)ioapic->ioapic_address, IOAPIC_REG_VERSION) >> 16 & 0xFF;
        entries++;

        // Check if the IRQ (NOT THE INTERRUPT VECTOR) is within the range of this I/O APIC
        if (irq >= ioapic->global_system_interrupt_base && irq < ioapic->global_system_interrupt_base + entries) {
            uint8_t entry = irq - ioapic->global_system_interrupt_base;

            uint64_t redtbl = ioapic_read_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry);

            kprintf("Original REDTBL: %x\n", redtbl);

            if (mask) {
                redtbl |= IOAPIC_REDIR_MASK;
            } else {
                redtbl &= ~IOAPIC_REDIR_MASK;
            }

            ioapic_write_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry, redtbl);

            redtbl = ioapic_read_redtbl((void*)(uint64_t)ioapic->ioapic_address, entry);

            kprintf("New REDTBL: %x\n", redtbl);

            return;
        }

        curr = (void*)((size_t)curr + len);
    }
}
