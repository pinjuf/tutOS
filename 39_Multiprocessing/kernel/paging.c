#include "paging.h"

#include "mm.h"
#include "util.h"

void init_paging(void) {
    uint64_t * pml4t = get_pml4t();

    // stage2 has already set up 4 Mibs of ID-mapped memory for the kernel, we only set up the HEAP
    // When using 0xB000000 as the virtual heap, we need to use PDPT we already have
    
    uint16_t heap_pml4t_index = (HEAP_VIRT >> 39) & 0x1FF;
    uint16_t heap_pdpt_index  = (HEAP_VIRT >> 30) & 0x1FF;
    uint16_t heap_pdt_index   = (HEAP_VIRT >> 21) & 0x1FF;

    uint64_t * pdpt = (uint64_t *)(pml4t[heap_pml4t_index] & ~0xFFF);
    uint64_t * pdt = (uint64_t *) HEAP_PAGETABLES_START;

    if (!(pml4t[heap_pml4t_index] & PAGE_PRESENT)) { // Do we need a new PDPT?
        pdpt = (uint64_t *)HEAP_PAGETABLES_START;
        pdt = (uint64_t*)(HEAP_PAGETABLES_START + PAGE_SIZE);
        pml4t[heap_pml4t_index] = (uint64_t)pdpt | HEAP_FLAGS;
    } else if (HEAP_FLAGS & PAGE_USER) {
        pml4t[heap_pml4t_index] |= PAGE_USER;
    }

    size_t pdts = HEAP_PTS/PAGE_ENTRIES + 1; // How many PDTs will we need, at least? (we need to enter them into the PDPT)

    for (size_t i = 0; i < pdts; i++) {
        pdpt[heap_pdpt_index + i] = ((uint64_t)pdt + PAGE_SIZE * i) | HEAP_FLAGS;
    }

    for (size_t i = 0; i < HEAP_PTS; i++) {
        pdt[heap_pdt_index + i] = (HEAP_PHYS + PAGE_ENTRIES * PAGE_SIZE * i) | HEAP_FLAGS | PAGE_PS;
    }

    asm volatile("mov %0, %%cr3" : : "a" (pml4t));
}

void * virt_to_phys(void * virt) {
    uint16_t pml4t_index = ((uint64_t)virt >> 39) & 0x1FF;
    uint16_t pdpt_index  = ((uint64_t)virt >> 30) & 0x1FF;
    uint16_t pdt_index   = ((uint64_t)virt >> 21) & 0x1FF;
    uint16_t pt_index    = ((uint64_t)virt >> 12) & 0x1FF;

    uint64_t * pml4t = get_pml4t();
    uint64_t * pdpt;
    uint64_t * pdt;
    uint64_t * pt;

    if ((uint64_t)pml4t >= 0x400000)
        pml4t = (void*)((uint64_t)pml4t - HEAP_PHYS + HEAP_VIRT);

    if (pml4t[pml4t_index] & PAGE_PRESENT) {
        if (pml4t[pml4t_index] & PAGE_PS) {
            return (void*) ((pml4t[pml4t_index] & ~0x7FFFFFFFFF) | ((uint64_t)virt & 0x7FFFFFFFFF));
        }

        pdpt = (void*) (pml4t[pml4t_index] & ~0xFFF);

        if ((uint64_t)pdpt >= 0x400000) { // If it is above kernel mem, it is part of the heap and must be translated to a virtual address
            pdpt = (void*)((uint64_t)pdpt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        return NULL;
    }

    if (pdpt[pdpt_index] & PAGE_PRESENT) {
        if (pdpt[pdpt_index] & PAGE_PS) {
            return (void*) ((pdpt[pdpt_index] & ~0x3FFFFFFF) | ((uint64_t)virt & 0x3FFFFFFF));
        }

        pdt = (void*) (pdpt[pdpt_index] & ~0xFFF);

        if ((uint64_t)pdt >= 0x400000) {
            pdt = (void*)((uint64_t)pdt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        return NULL;
    }

    if (pdt[pdt_index] & PAGE_PRESENT) {
        if (pdt[pdt_index] & PAGE_PS) {
            return (void*) ((pdt[pdt_index] & ~0x1FFFFF) | ((uint64_t)virt & 0x1FFFFF));
        }

        pt = (void*) (pdt[pdt_index] & ~0xFFF);

        if ((uint64_t)pt >= 0x400000) {
            pt = (void*)((uint64_t)pt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        return NULL;
    }

    if (pt[pt_index] & PAGE_PRESENT) {
        return (void*) ((pt[pt_index] & ~0xFFF) | ((uint64_t)virt & 0xFFF));
    }

    return NULL;
}

void mmap_page(void * virt, void * phys, uint64_t attr) {
    uint64_t * pml4t = get_pml4t();

    // Anyone who calls _mmap_page should know what they are doing
    if (((uint64_t)virt >= HEAP_VIRT) && ((uint64_t)virt < (HEAP_VIRT+HEAP_PTS*PAGE_ENTRIES*PAGE_SIZE))) {
        kwarn(__FILE__,__func__,"refusing to remap heap virtual memory");
        return;
    }

    // Check if the virt address is valid
    if ((uint64_t)virt < 0x400000) {
        kwarn(__FILE__,__func__,"refusing to remap kernel virtual memory");
        return;
    }

    if ((uint64_t)pml4t >= 0x400000)
        pml4t = (void*)((uint64_t)pml4t - HEAP_PHYS + HEAP_VIRT);

    _mmap_page(pml4t, virt, phys, attr);

    asm volatile("invlpg (%0)" : : "r"(virt));
}

void _mmap_page(uint64_t * pml4t, void * virt, void * phys, uint64_t attr) {
    // Note: We assume we are only using 4KiB-pages (not PS bits)

    // Addresses must be cannonical in Long Mode (upper 17 bits must be the same)
    uint64_t upper = (uint64_t)virt >> 47;
    if (!(upper == 0x00000 || upper == 0x1FFFF)) {
        kwarn(__FILE__,__func__,"refusing to map to non-canonical virtual address");
        return;
    }

    if ((uint64_t)virt & 0xFFF) {
        kwarn(__FILE__,__func__,"virtual address not page-aligned");
    }

    if ((uint64_t)phys & 0xFFF) {
        kwarn(__FILE__,__func__,"physical address not page-aligned");
    }

    uint16_t pml4t_index = ((uint64_t)virt >> 39) & 0x1FF;
    uint16_t pdpt_index  = ((uint64_t)virt >> 30) & 0x1FF;
    uint16_t pdt_index   = ((uint64_t)virt >> 21) & 0x1FF;
    uint16_t pt_index    = ((uint64_t)virt >> 12) & 0x1FF;

    uint64_t * pdpt;
    uint64_t * pdt;
    uint64_t * pt;

    // Does the needed PDPT exist?
    if (pml4t[pml4t_index] & PAGE_PRESENT) {
        if (attr & PAGE_USER) // U/S needs to be set in all levels
            pml4t[pml4t_index] |= PAGE_USER;

        pdpt = (uint64_t *)(pml4t[pml4t_index] & ~0xFFF);

        if ((uint64_t)pdpt >= 0x400000) { // If it is above kernel mem, it is part of the heap and must be translated to a virtual address
            pdpt = (void*)((uint64_t)pdpt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        pdpt = calloc_pages(1);

        pml4t[pml4t_index] = (uint64_t)virt_to_phys(pdpt) | attr;
    }

    if (pdpt[pdpt_index] & PAGE_PRESENT) {
        if (attr & PAGE_USER)
            pdpt[pdpt_index] |= PAGE_USER;

        pdt = (uint64_t *)(pdpt[pdpt_index] & ~0xFFF);

        if ((uint64_t)pdt >= 0x400000) {
            pdt = (void*)((uint64_t)pdt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        pdt = calloc_pages(1);

        pdpt[pdpt_index] = (uint64_t)virt_to_phys(pdt) | attr;
    }

    if (pdt[pdt_index] & PAGE_PRESENT) {
        if (attr & PAGE_USER)
            pdt[pdt_index] |= PAGE_USER;

        pt = (uint64_t *)(pdt[pdt_index] & ~0xFFF);

        if ((uint64_t)pt >= 0x400000) {
            pt = (void*)((uint64_t)pt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        pt = calloc_pages(1);

        pdt[pdt_index] = (uint64_t)virt_to_phys(pt) | attr;
    }

    pt[pt_index] = (uint64_t)phys | attr;
}

void mmap_page_2mb(void * virt, void * phys, uint64_t attr) {
    uint64_t * pml4t = get_pml4t();

    // Anyone who calls _mmap_page should know what they are doing
    if (((uint64_t)virt >= HEAP_VIRT) && ((uint64_t)virt < (HEAP_VIRT+HEAP_PTS*PAGE_ENTRIES*PAGE_SIZE))) {
        kwarn(__FILE__,__func__,"refusing to remap heap virtual memory");
        return;
    }

    // Check if the virt address is valid
    if ((uint64_t)virt < 0x400000) {
        kwarn(__FILE__,__func__,"refusing to remap kernel virtual memory");
        return;
    }

    if ((uint64_t)pml4t >= 0x400000)
        pml4t = (void*)((uint64_t)pml4t - HEAP_PHYS + HEAP_VIRT);

    _mmap_page_2mb(pml4t, virt, phys, attr);

    asm volatile("invlpg (%0)" : : "r"(virt));
}

void _mmap_page_2mb(uint64_t * pml4t, void * virt, void * phys, uint64_t attr) {
    uint16_t pml4t_index = ((uint64_t)virt >> 39) & 0x1FF;
    uint16_t pdpt_index  = ((uint64_t)virt >> 30) & 0x1FF;
    uint16_t pdt_index   = ((uint64_t)virt >> 21) & 0x1FF;
    uint16_t pt_index    = ((uint64_t)virt >> 12) & 0x1FF;

    if (pt_index) {
        kwarn(__FILE__,__func__,"virtual address not 2MiB-aligned");
        return;
    }

    uint64_t * pdpt;
    uint64_t * pdt;

    // Does the needed PDPT exist?
    if (pml4t[pml4t_index] & PAGE_PRESENT) {
        if (attr & PAGE_USER) // U/S needs to be set in all levels
            pml4t[pml4t_index] |= PAGE_USER;

        pdpt = (uint64_t *)(pml4t[pml4t_index] & ~0xFFF);

        if ((uint64_t)pdpt >= 0x400000) { // If it is above kernel mem, it is part of the heap and must be translated to a virtual address
            pdpt = (void*)((uint64_t)pdpt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        pdpt = calloc_pages(1);

        pml4t[pml4t_index] = (uint64_t)virt_to_phys(pdpt) | attr;
    }

    if (pdpt[pdpt_index] & PAGE_PRESENT) {
        if (attr & PAGE_USER)
            pdpt[pdpt_index] |= PAGE_USER;

        pdt = (uint64_t *)(pdpt[pdpt_index] & ~0xFFF);

        if ((uint64_t)pdt >= 0x400000) {
            pdt = (void*)((uint64_t)pdt - HEAP_PHYS + HEAP_VIRT);
        }
    } else {
        pdt = calloc_pages(1);

        pdpt[pdpt_index] = (uint64_t)virt_to_phys(pdt) | attr;
    }

    pdt[pdt_index] = (uint64_t)phys | attr | PAGE_PS;
}

void mmap_pages(void * virt, void * phys, uint64_t attr, size_t count) {
    for (size_t i = 0; i < count; i++)
        mmap_page((void*)((uint64_t)virt + i * PAGE_SIZE), (void*)((uint64_t)phys + i * PAGE_SIZE), attr);
}
