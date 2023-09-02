#include "elf.h"
#include "mm.h"
#include "util.h"
#include "vfs.h"

int elf_load(process_t * out, void * buf, size_t stack_pages, bool kmode) {
    elf64_hdr_t * hdr = buf;

    if (*(uint32_t*)&(hdr->e_ident[ELF_EI_MAG0]) != ELF_MAGIC) {
        kwarn(__FILE__,__func__,"wrong elf magic");
        return 1;
    }

    if (hdr->e_machine != ELF_X86_64) {
        kwarn(__FILE__,__func__,"not x86_64");
        return 1;
    }

    if (hdr->e_type != ELF_EXEC) {
        kwarn(__FILE__,__func__,"not an executable");
        return 1;
    }

    out->regs.rip = hdr->e_entry;

    out->regs.rsp = DEF_ELF_RSP + stack_pages * PAGE_SIZE;
    out->stack_pages = stack_pages;
    out->stack_heap = alloc_pages(stack_pages);
    out->regs.rbp = out->regs.rsp;

    out->regs.cr3 = (uint64_t)get_pml4t();
    out->regs.rflags = 0x202; // IF
    out->regs.cs = kmode ? (1*8) : ((6*8) | 3);
    out->regs.ss = kmode ? (2*8) : ((5*8) | 3);

    out->kmode = kmode;

    out->pagemaps = kmalloc(sizeof(pagemap_t) * (hdr->e_phnum + 1)); // Some overhead, I know...
    out->pagemaps_n = 0;

    // Set up a stack pagemap
    out->pagemaps[out->pagemaps_n].attr = PAGE_PRESENT | PAGE_RW | PAGE_USER;
    out->pagemaps[out->pagemaps_n].virt = (void*)DEF_ELF_RSP;
    out->pagemaps[out->pagemaps_n].phys = virt_to_phys(out->stack_heap);
    out->pagemaps[out->pagemaps_n].n    = stack_pages;
    out->pagemaps_n++;

    for (size_t i = 0; i < hdr->e_phnum; i++) {
        elf64_phdr_t * phdr = (void*)((size_t)hdr + hdr->e_phoff + hdr->e_phentsize * i);

        if (phdr->p_type != ELF_PT_LOAD)
            continue;

        if (phdr->p_vaddr < HEAP_PHYS) {
            kwarn(__FILE__,__func__,"refusing to map to low kernel mem");
            continue;
        }

        out->pagemaps[out->pagemaps_n].virt = (void*)phdr->p_vaddr;
        out->pagemaps[out->pagemaps_n].attr = PAGE_PRESENT | PAGE_RW | PAGE_USER;

        size_t allocsz = phdr->p_filesz;
        if (phdr->p_memsz > phdr->p_filesz) {
            allocsz = phdr->p_memsz;
        }

        size_t pages = allocsz / PAGE_SIZE;
        if (allocsz % PAGE_SIZE)
            pages++;

        out->pagemaps[out->pagemaps_n].n = pages;

        void * buf = calloc_pages(pages);

        memcpy(buf, (void*)((size_t)hdr + phdr->p_offset), phdr->p_filesz);

        out->pagemaps[out->pagemaps_n].phys = virt_to_phys(buf);

        out->pagemaps_n++;
    }

    return 0;
}
