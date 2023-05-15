#include "elf.h"
#include "mm.h"
#include "util.h"

process_t * elf_load(void * buf, size_t stacksize, bool kmode) {
    elf64_hdr_t * hdr = buf;

    if (*(uint32_t*)&(hdr->e_ident[ELF_EI_MAG0]) != ELF_MAGIC) {
        kwarn(__FILE__,__func__,"wrong elf magic");
        return NULL;
    }

    if (hdr->e_machine != ELF_X86_64) {
        kwarn(__FILE__,__func__,"not x86_64");
        return NULL;
    }

    if (hdr->e_type != ELF_EXEC) {
        kwarn(__FILE__,__func__,"not an executable");
        return NULL;
    }

    process_t * out = NULL;
    for (size_t i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_NONE) {
            out = &processes[i];
        }
    }

    if (out == NULL) {
        kwarn(__FILE__,__func__,"no free process slot");
        return NULL;
    }

    memset(out, 0, sizeof(process_t));

    out->regs.rip = hdr->e_entry;
    out->regs.rsp = (size_t)kmalloc(stacksize) + stacksize;
    out->regs.rbp = out->regs.rsp;
    asm volatile ("mov %%cr3, %0" : "=a" (out->regs.cr3));
    out->regs.rflags = 0x202; // IF
    out->regs.cs = kmode ? (1*8) : ((6*8) | 3);
    out->regs.ss = kmode ? (2*8) : ((5*8) | 3);

    out->pagemaps = kmalloc(sizeof(pagemap_t) * hdr->e_phnum); // Some overhead, I know...

    for (size_t i = 0; i < hdr->e_phnum; i++) {
        elf64_phdr_t * phdr = (void*)((size_t)hdr + hdr->e_phoff + hdr->e_phentsize * i);

        if (phdr->p_type != ELF_PT_LOAD)
            continue;

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

    out->state = PROCESS_NOT_RUNNING;

    return out;
}
