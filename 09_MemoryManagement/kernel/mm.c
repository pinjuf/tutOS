#include "mm.h"

#include "util.h"

uint16_t * mm_bitmap = (uint16_t*) 0x140000;

void init_mm(void) {
    memset(mm_bitmap, 0, MM_SIZE);
}

bool mm_is_used(size_t chunk) {
    uint16_t m = mm_bitmap[chunk / 16];
    uint8_t  p = chunk % 16;
    return (m & (1 << p));
}

void mm_set_used(size_t chunk, bool used) {
    uint16_t m = mm_bitmap[chunk / 16];
    uint8_t  p = chunk % 16;
    if (used) {
        mm_bitmap[chunk / 16] = m | (1 << p);
    } else {
        mm_bitmap[chunk / 16] = m & ~(1 << p);
    }
}

void * alloc_pages(size_t n) {
    size_t start = 0;
    size_t got = 0;

    for (size_t i = 0; i < MM_LENGTH; i++) {
        if (mm_bitmap[i] == 0x0000) {
            if (got == 0)
                start = i;

            got++;
        } else {
            got = 0;
        }

        if (got == n) {
            for (size_t i = 0; i < n; i++)
                mm_bitmap[start + i] = 0xFFFF;

            return (void*)(HEAP_VIRT + PAGE_SIZE * start);
        }
    }

    return NULL;
}

void free_pages(void * ptr, size_t n) {
    size_t i = ((uint64_t)ptr - HEAP_VIRT) / PAGE_SIZE;

    for (size_t j = 0; j < n; j++) {
        if (mm_bitmap[i + j] != 0xFFFF) {
            kwarn(__FILE__, __LINE__, "page not in use");
        }

        mm_bitmap[i + j] = 0x0000;
    }
}

size_t mm_first_free_chunk(void) {
    for (size_t i = 0; i < MM_LENGTH; i++) {
        if (mm_bitmap[i] != 0xFFFF) {
            for (uint8_t p = 0; p < 16; p++) {
                if (!mm_is_used(i * 16 + p)) {
                    return i * 16 + p;
                }
            }
        }
    }

    return SIZE_MAX;
}

void * kmalloc(size_t n) {
    n += 12; // Space for our header

    size_t needed = n / MM_CHUNKSIZE;
    if (n % MM_CHUNKSIZE)
        needed++;

    size_t got   = 0;
    size_t i     = mm_first_free_chunk(); // Just to speed things up a bit

    size_t start = i;

    while (i < (MM_LENGTH * 16)) {
        if (!mm_is_used(i)) {
            if (got == 0)
                start = i;

            got++;
        } else {
            got = 0;
        }

        if (got == needed) {
            for (size_t j = 0; j < needed; j++) {
                mm_set_used(start + j, 1);
            }

            void * ptr = (void*)(HEAP_VIRT + MM_CHUNKSIZE * start);

            *(uint32_t*)ptr = MM_MAGIC;
            ptr = (void*) ((uint64_t)ptr + sizeof(uint32_t));
            *(size_t*)ptr = needed;
            ptr = (void*) ((uint64_t)ptr + sizeof(size_t));

            return ptr;
        }

        i++;
    }

    return NULL;
}

void kfree(void * ptr) {
    ptr = (void*) ((uint64_t)ptr - sizeof(size_t));
    size_t n = *(uint64_t*)ptr;
    ptr = (void*) ((uint64_t)ptr - sizeof(uint32_t));
    if (*(uint32_t*)ptr != MM_MAGIC) {
        kwarn(__FILE__,__LINE__,"no malloc signature");
    }

    size_t start = ((uint64_t)ptr - HEAP_VIRT) / MM_CHUNKSIZE;
    for (size_t i = 0; i < n; i++) {
        mm_set_used(start + i, 0);
    }
}
