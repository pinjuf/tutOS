#pragma once

#include "types.h"

typedef struct rsdp_t {
    char sign[8];
    uint8_t chksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr;
} __attribute__((packed)) rsdp_t;

typedef struct xsdp_t {
    char sign[8];
    uint8_t chksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr;

    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_chksum;
    uint8_t reserved[3];
} __attribute__((packed)) xsdp_t;

typedef struct acpi_sdt_t {
    char sign[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    char oemid[6];
    char oem_tableid[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_t;

typedef struct rsdt_t {
    acpi_sdt_t sdt;
    uint32_t sdt_ptrs[];
} __attribute__((packed)) rsdt_t;

typedef struct xsdt_t {
    acpi_sdt_t sdt;
    uint64_t sdt_ptrs[];
} __attribute__((packed)) xsdt_t;

extern rsdp_t * rsdp;
extern xsdp_t * xsdp;

extern rsdt_t * rsdt;
extern xsdt_t * xsdt;

extern size_t acpi_sdt_count;

void init_acpi(void);

#define RSDP_SEARCH_START 0x000E0000
#define RSDP_SEARCH_END   0x00100000
#define RSDP_SEARCH_ALIGN 16

#define RSDP_SIGNATURE "RSD PTR "
