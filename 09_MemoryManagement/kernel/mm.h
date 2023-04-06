#pragma once

#include "types.h"
#include "paging.h"

#define MM_CHUNKSIZE (PAGE_SIZE/(sizeof(uint16_t)*8))

#define MM_LENGTH (HEAP_PTS * PAGE_ENTRIES)
#define MM_SIZE   (HEAP_PTS * PAGE_ENTRIES * sizeof(uint16_t))

#define MM_MAGIC 0xFA7A5569 // Fatass 69

extern uint16_t * mm_bitmap;

void init_mm(void);

void * alloc_pages(size_t n);
void free_pages(void * ptr, size_t n);

size_t mm_first_free_chunk(void);
bool mm_is_used(size_t chunk);

void * kmalloc(size_t n);
void kfree(void * ptr);
