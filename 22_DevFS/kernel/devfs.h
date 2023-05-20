#pragma once

#include "gpt.h"

enum DEVFS_DEV {
    DEVFS_UNKN,
    DEVFS_DIR, // The /dev directory itself
    DEVFS_VESA,
    DEVFS_PCSPK,
    DEVFS_HDD,
    DEVFS_TTY,
    DEVFS_MEM,
};

typedef struct devfs_file_t {
    enum DEVFS_DEV type;
    part_t p; // Only used if this is a DEVFS_HDD
} devfs_file_t;

// devfs doesn't need a getdevfs initialization (for now)
void * devfs_getfile(void *, char * path, int);
void devfs_closefile(void * f);
size_t devfs_readfile(void * f, void * buf, size_t count);
size_t devfs_writefile(void * f, void * buf, size_t count);
