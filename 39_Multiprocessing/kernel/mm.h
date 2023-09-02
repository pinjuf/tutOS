#pragma once

#include "types.h"
#include "paging.h"
#include "util.h"

#define MM_CHUNKSIZE (PAGE_SIZE/(sizeof(uint16_t)*8))

#define MM_LENGTH (HEAP_PTS * PAGE_ENTRIES)
#define MM_SIZE   (HEAP_PTS * PAGE_ENTRIES * sizeof(uint16_t))

#define MM_MAGIC 0xFA7A5569 // Fatass 69

// Big dumb memory bitmap system
extern uint16_t * mm_bitmap;
extern spinlock_t mm_bitmap_lock;

void init_mm(void);

void * alloc_pages(size_t n);
void * calloc_pages(size_t n);
void free_pages(void * ptr, size_t n);

size_t mm_first_free_chunk(void);
bool mm_is_used(size_t chunk);

void * kmalloc(size_t n);
void * kcalloc(size_t n);
void kfree(void * ptr);
void * krealloc(void * ptr, size_t n);
