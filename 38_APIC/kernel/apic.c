#include "apic.h"
#include "util.h"
#include "acpi.h"
#include "mm.h"

void * ioapic_base = NULL;
cpu_coreinfo_t * coreinfos = NULL;
size_t cpu_cores = 0;

uint32_t ioapic_read(uint32_t reg) {
    uint32_t volatile * ioapic = (uint32_t volatile *)ioapic_base;
    ioapic[0] = reg & 0xFF;
    return ioapic[4];
}

void ioapic_write(uint32_t reg, uint32_t val) {
    uint32_t volatile * ioapic = (uint32_t volatile *)ioapic_base;
    ioapic[0] = reg & 0xFF;
    ioapic[4] = val;
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
    
    // Get some info about the cores
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

                if (!ioapic_base) {
                    ioapic_base = (void*)(uint64_t)ioapic->ioapic_address;
                } else {
                    kwarn(__FILE__,__func__,"multiple I/O APICs not supported");
                }

                kprintf("GSI base: %u\n", ioapic->global_system_interrupt_base);

                break;
            }
            case MADT_TYPE_ISO: {
                madt_iso_t * iso = curr;

                kprintf("IRQ %u: %u\n", iso->irq_source, iso->global_system_interrupt);

                break;
            }
            default:
                break;
        }

        curr = (void*)((size_t)curr + len);
    }

    mmap_page((void*)APIC_BASE, (void*)APIC_BASE, PAGE_PRESENT | PAGE_RW);
    // Enable the APIC by setting the APIC enable bit in the spurious interrupt vector register
    *(uint32_t*)(APIC_BASE + 0xF0) |= 1 << 8 | 0xFF; // 0xFF is the spurious interrupt vector

    // IOAPIC configuration
    mmap_page(ioapic_base, ioapic_base, PAGE_PRESENT | PAGE_RW);
    uint32_t ioapic_count = (ioapic_read(0x01) >> 16) + 1;

    for (size_t i = 0; i < ioapic_count; i++) {
        // Set up this IOREDTBL

        uint32_t low32 = 0x20 + i; // IRQ 0x20 is the first IRQ that is not reserved by the CPU, no other flags
        uint32_t high32 = bsp << 24; // Set the destination to the BSP

        if (i == 2) { // TODO: Handle ISOs
            low32 = 0x20;
        }

        ioapic_write(0x10 + i * 2, low32);
        ioapic_write(0x10 + i * 2 + 1, high32);
    }
}
