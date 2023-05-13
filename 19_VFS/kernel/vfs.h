#pragma once

#include "types.h"
#include "gpt.h"
#include "fat.h"
#include "ext2.h"

#define DIRSEP '/'

enum FILETYPE {
    FILE_UNKN,
    FILE_REG,
    FILE_DIR,
    // TODO: Support more filetypes
};

enum FILEMODE {
    FILE_R = 0,
    FILE_W,
};

typedef struct filehandle_t {
    size_t mountpoint; // Offset into the mountpoint table
    enum FILETYPE type;
    enum FILEMODE mode;
    void * internal_file;
    size_t size;
    size_t curr;       // Current offset
} filehandle_t;

// Generic structure describing an FS driver
typedef struct filesystem_t {
    char name[8];
    void * (* get_fs)(part_t * p);

    filehandle_t * (* get_filehandle)(void * internal_fs, char * path, enum FILEMODE mode);
    void (* close_filehandle)(filehandle_t * f);

    size_t (* read_file)(filehandle_t * f, void * buf, size_t count);
} filesystem_t;

enum FILESYSTEM {
    FS_UNKN,
    FS_EXT2,
    FS_FAT32,
};

typedef struct mountpoint_t {
    enum FILESYSTEM type;
    void * internal_fs; // ext2fs_t, fat32fs_t, etc.
    part_t * p;         // Might not be used on special filesystems
    char * path;        // Must end with "/"
} mountpoint_t;

// Needs to be in the same order as FILESYSTEM!
static const filesystem_t FILESYSTEMS[] = {
    {"unkn", NULL, NULL, NULL, NULL}, // Unknown FS

    {"ext2",
        (void* (*) (part_t * p)) get_ext2fs,
        (filehandle_t * (*) (void * internal_fs, char * path, enum FILEMODE mode)) ext2_getfile,
        (void (*) (filehandle_t * f)) ext2_closefile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) ext2_readfile,
    },

    {"fat32",
        (void* (*) (part_t * p)) get_fat32fs,
        NULL, // TODO: Implement fat32_getfile etc.
        NULL,
        NULL,
    },
};

extern mountpoint_t * mountpoints;
#define MOUNTPOINTS_N 4

void init_vfs();
filehandle_t * kopen(char * path, enum FILEMODE FILE_R);
void kclose(filehandle_t * f);
size_t kread(filehandle_t * f, void * buf, size_t count);
