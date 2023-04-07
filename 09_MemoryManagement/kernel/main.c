#include "main.h"
#include "paging.h"
#include "vga.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"

void _kmain() {
    // Set up our stack
    asm volatile(" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    // Set up VGA
    vga_enable_cursor(0, 16);
    vga_get_cursor();

    // I really like cool sounding logs
    kputs("KRN OK\n");

    init_kgdt();
    kputs("GDT OK\n");

    init_idt();
    kputs("IDT OK\n");

    init_paging();
    kputs("PGN OK\n");

    init_mm();
    kputs("MM  OK\n");

    init_pic();
    kputs("PIC OK\n");

    init_pit0(PIT0_FREQ); // Interrupt every 4ms
    kputs("PIT OK\n");

    kputs("KRN MN\n");

    // Please ignore the copious newlines
    kputs("\nvirt_to_phys() demo:\n");

    kputs("Heap start virtual: 0x");
    kputhex((uint64_t)HEAP_VIRT);
    kputs("\nHeap end virtual: 0x");
    kputhex((uint64_t)HEAP_VIRT + HEAP_PTS * PAGE_ENTRIES * PAGE_SIZE - 1);
    kputs("\nHeap start physical: 0x");
    kputhex((uint64_t)virt_to_phys((void*)HEAP_VIRT));
    kputs("\nHeap end physical: 0x");
    kputhex((uint64_t)virt_to_phys((void*)HEAP_VIRT + HEAP_PTS * PAGE_ENTRIES * PAGE_SIZE - 1));
    kputs("\nHeap end +1 physical: 0x");
    kputhex((uint64_t)virt_to_phys((void*)HEAP_VIRT + HEAP_PTS * PAGE_ENTRIES * PAGE_SIZE));

    kputs("\n\nkmalloc() / kfree() demo:");

    void * m1 = kmalloc(0x100);
    kputs("\nAllocated 256 bytes m1 at 0x");
    kputhex((uint64_t)m1);

    void * m2 = kmalloc(0x20);
    kputs("\nAllocated 32 bytes m2 at 0x");
    kputhex((uint64_t)m2);

    void * m3 = alloc_pages(1);
    kputs("\nAllocated a page m3 at 0x");
    kputhex((uint64_t)m3);

    void * m4 = kmalloc(0x200);
    kputs("\nAllocated 512 bytes m4 at 0x");
    kputhex((uint64_t)m4);

    void * m5 = kmalloc(0x1000);
    kputs("\nAllocated 4096 bytes m5 at 0x");
    kputhex((uint64_t)m5);

    kfree(m2);
    kputs("\nFreed m2");

    void * m6 = kmalloc(0x50);
    kputs("\nAllocated 80 bytes m6 at 0x");
    kputhex((uint64_t)m6);

    kfree(m4);
    kputs("\nFreed m4");

    void * m7 = kmalloc(0x10000);
    kputs("\nAllocated 65536 bytes m7 at 0x");
    kputhex((uint64_t)m7);

    kfree(m1);
    free_pages(m3, 1);
    kfree(m5);
    kfree(m6);
    kfree(m7);
    kputs("\nFreed m1, m3, m5, m6, m7\n\n");

    kputs("mmap_page demo:\n");

    void * page = alloc_pages(1);
    char msg[] = "Hello from mmapped memory!\n";

    kputs("Allocated page at 0x");
    kputhex((uint64_t)page);
    kputs(" (phys=0x");
    kputhex((uint64_t)virt_to_phys(page));
    kputs(")\n");

    memcpy(page, msg, strlen(msg));

    kputs("Reading from page original virt:\n");
    kputs(page);

    void * my_virt1 = (void*)0xF0000000;
    void * my_virt2 = (void*)0xF0001000;

    mmap_page(my_virt1, virt_to_phys(page), PAGE_PRESENT|PAGE_RW);
    kputs("Mapped 0x");
    kputhex((uint64_t)my_virt1);
    kputs(" to phys=0x");
    kputhex((uint64_t)virt_to_phys(page));
    kputs("\nReading from new virt:\n");
    kputs(my_virt1);

    mmap_page(my_virt2, virt_to_phys(page), PAGE_PRESENT|PAGE_RW);
    kputs("Mapped 0x");
    kputhex((uint64_t)my_virt2);
    kputs(" to phys=0x");
    kputhex((uint64_t)virt_to_phys(page));
    kputs("\nReading from new virt:\n");
    kputs(my_virt2);

    kputc('\n');

    sti;

    kputs("KRN DN\n");
    while (1);
}
