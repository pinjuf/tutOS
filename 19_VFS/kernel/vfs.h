#pragma once

#include "types.h"
#include "gpt.h"
#include "fat.h"
#include "ext2.h"

typedef struct filehandle_t {
    size_t mountpoint; // Offset into the mountpoint table
    void * internal_file;
    size_t size;
    size_t curr;       // Current offset
} filehandle_t;

// Generic structure describing an FS driver
typedef struct filesystem_t {
    char name[8];
    void * (* get_fs)(part_t * p);
} filesystem_t;

enum FILESYSTEM {
    FS_UNKN,
    FS_EXT2,
    FS_FAT32,
};

typedef struct mountpoint_t {
    enum FILESYSTEM type;
    void * internal_fs; // ext2fs_t, fat32fs_t, etc.
    part_t * p;
    char * path;
} mountpoint_t;

// Needs to be in the same order as FILESYSTEM!
static const filesystem_t FILESYSTEMS[] = {
    {"unkn", NULL}, // Unknown FS

    {"ext2",
        (void* (*) (part_t * p)) get_ext2fs},

    {"fat32",
        (void* (*) (part_t * p)) get_fat32fs},
};

extern mountpoint_t * mountpoints;
#define MOUNTPOINTS_N 1

void init_vfs();
