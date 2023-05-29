#pragma once

#include "types.h"
#include "schedule.h"

typedef struct elf64_hdr_t {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf64_hdr_t;

typedef struct elf64_phdr_t {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf64_phdr_t;

#define ELF_MAGIC 0x464c457F
#define ELF_32 1
#define ELF_64 2
#define ELF_LTLEND 1
#define ELF_BIGEND 2

enum ELF_ABI {
    ELF_SYSV,
    ELF_HPUX,
    ELF_NETBSD,
    ELF_LINUX,
    ELF_HURD,
    ELF_SOLARIS,
    ELF_AIX,
    ELF_IRIX,
    ELF_FREEBSD,
    ELF_TRU64,
    ELF_NOVMOD,
    ELF_OPENBSD,
    ELF_OPENVMS,
    ELF_NONSTOP,
    ELF_AROS,
    ELF_FENIX,
    ELF_NUXI,
    ELF_STRATUS,
};

enum ELF_TYPE {
    ELF_UNKN,
    ELF_REL,
    ELF_EXEC,
    ELF_DYN,
    ELF_CORE,
};

enum ELF_MACHINE {
    ELF_NONE,
    ELF_X86    = 0x3,
    ELF_IA32   = 0x32,
    ELF_X86_64 = 0x3E,
};

enum ELF_IDENT {
    ELF_EI_MAG0,
    ELF_EI_MAG1,
    ELF_EI_MAG2,
    ELF_EI_MAG3,
    ELF_EI_CLASS,
    ELF_EI_DATA,
    ELF_EI_VERSION,
    ELF_EI_OSABI,
    ELF_EI_ABIVERSION,
};

enum ELF_PTYPE {
    ELF_PT_NULL,
    ELF_PT_LOAD,
    ELF_PT_DYNAMIC,
    ELF_PT_INTERP,
    ELF_PT_NOTE,
    ELF_PT_SHLIB,
    ELF_PT_PHDR,
    ELF_PT_TLS,
};

int elf_load(process_t * out, void * buf, size_t stack_pages, bool kmode);

#define DEF_ELF_RSP 0x7FFF00000000
