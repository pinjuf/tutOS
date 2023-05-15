#include "elf.h"
#include "util.h"

process_t * elf_load(void * buf, bool kmode) {
    elf64_hdr_t * hdr = buf;

    if (*(uint32_t*)&(hdr->e_ident[0]) != ELF_MAGIC) {
        kwarn(__FILE__,__func__,"wrong elf magic");
        return NULL;
    }
}
