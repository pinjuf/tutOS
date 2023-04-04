#include "paging.h"

#include "util.h"

void init_paging(void) {
    uint64_t * pml4t;
    asm ("mov %%cr3, %0" : "=a" (pml4t));

    // stage2 has already set up 4 Mibs of ID-mapped memory for the kernel, we only set up the HEAP
    // When using 0xB000000 as the virtual heap, we need to use PDPT we already have
    
    uint16_t heap_pml4t_index = (HEAP_VIRT >> 39) & 0x1FF; // Should be 0 (TODO: What if it isn't ;))
    uint16_t heap_pdpt_index  = (HEAP_VIRT >> 30) & 0x1FF;
    uint16_t heap_pdt_index   = (HEAP_VIRT >> 21) & 0x1FF;

    uint64_t * pdpt = (uint64_t *)(pml4t[heap_pml4t_index] & ~0x1FF);
    uint64_t * pdt = (uint64_t *) HEAP_PAGETABLES_START;

    if (!(pml4t[heap_pml4t_index] & PAGE_PRESENT)) { // Do we need a new PDPT?
        pdpt = (uint64_t *)HEAP_PAGETABLES_START;
        pdt = (uint64_t*)(HEAP_PAGETABLES_START + PAGE_SIZE);
        pml4t[heap_pml4t_index] = (uint64_t)pdpt | HEAP_FLAGS;
    }

    size_t pdts = HEAP_PTS/PAGE_ENTRIES + 1; // How many PDTs will we need, at least? (we need to enter them into the PDPT)

    for (size_t i = 0; i < pdts; i++) {
        pdpt[heap_pdpt_index + i] = ((uint64_t)pdt + PAGE_SIZE * i) | HEAP_FLAGS;
    }

    for (size_t i = 0; i < HEAP_PTS; i++) {
        pdt[heap_pdt_index + i] = (HEAP_PHYS + PAGE_ENTRIES * PAGE_SIZE * i) | HEAP_FLAGS | PAGE_PS;
    }

    asm ("mov %0, %%cr3" : : "a" (pml4t));
}
