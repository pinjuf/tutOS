#include "acpi.h"
#include "util.h"
#include "paging.h"

rsdp_t * rsdp = NULL;
rsdt_t * rsdt = NULL;

size_t acpi_sdt_count = 0;

rsdp_t * find_rsdp() {
    rsdp_t * curr = (void*)RSDP_SEARCH_START;

    while ((uint64_t)curr < (uint64_t)RSDP_SEARCH_END) {
        if (strncmp(curr->sign, RSDP_SIGNATURE, 8) == 0) {
            if ((uint8_t)chksum8(curr, sizeof(rsdp_t)) == 0)
                return curr;
        }

        curr = (void*)((uint64_t)curr + RSDP_SEARCH_ALIGN);
    }

    return NULL;
}

void init_acpi(void) {
    rsdp = find_rsdp();
    if (rsdp == NULL) {
        kwarn(__FILE__,__func__,"ACPI RSDP not found");
    }

    if (rsdp->revision == 2) {
        kwarn(__FILE__,__func__,"ACPI XSDP not supported");
    }

    rsdt = (void*)(uint64_t)rsdp->rsdt_addr;
    void * rsdt_align = (void*)((uint64_t)rsdt & ~(PAGE_SIZE - 1));
    mmap_page(rsdt_align, rsdt_align, PAGE_PRESENT | PAGE_RW);

    acpi_sdt_count = (rsdt->sdt.length - sizeof(acpi_sdt_t)) / sizeof(uint32_t);
}

acpi_sdt_t * get_acpi_table(char * sign) {
    for (size_t i = 0; i < acpi_sdt_count; i++) {
        acpi_sdt_t * curr = (void*)(uint64_t)rsdt->sdt_ptrs[i];

        void * curr_align = (void*)((uint64_t)curr & ~(PAGE_SIZE - 1));
        mmap_page(curr_align, curr_align, PAGE_PRESENT | PAGE_RW);

        if (strncmp(curr->sign, sign, sizeof(curr->sign)))
            continue;

        size_t pages = curr->length / PAGE_SIZE;
        if (curr->length % PAGE_SIZE)
            pages++;

        mmap_pages(curr_align, curr_align, pages, PAGE_PRESENT | PAGE_RW);

        return curr;
    }

    return NULL;
}
