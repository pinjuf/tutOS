#pragma once

#include "types.h"

#define PAGE_PRESENT (1<<0)
#define PAGE_RW      (1<<1)
#define PAGE_USER    (1<<2)
#define PAGE_PS      (1<<7)
#define PAGE_GLOBAL  (1<<8)

#define PAGE_SIZE 0x1000
#define PAGE_ENTRIES 0x200

#define HEAP_PAGETABLES_START 0x120000 // Where to put our PDPT/PDTs for the heap memory
#define HEAP_PTS 128                   // Amount of full page tables (we use the size bit, so this is the amount of entries in the PDTS) (=256 Mibs)
#define HEAP_FLAGS (PAGE_PRESENT | PAGE_RW | PAGE_USER)

#define HEAP_PHYS ((uint64_t)0x400000)
#define HEAP_VIRT ((uint64_t)0xB0000000)

void init_paging(void);

void * virt_to_phys(void * virt);
void _mmap_page(uint64_t * pml4t, void * virt, void * phys, uint64_t attr);
void mmap_page(void * virt, void * phys, uint64_t attr);
void mmap_pages(void * virt, void * phys, uint64_t attr, size_t count);
void _mmap_page_2mb(uint64_t * pml4t, void * virt, void * phys, uint64_t attr);
void mmap_page_2mb(void * virt, void * phys, uint64_t attr);
