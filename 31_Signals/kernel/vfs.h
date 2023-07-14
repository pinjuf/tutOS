#pragma once

#include "types.h"
#include "gpt.h"
#include "fat.h"
#include "ext2.h"
#include "devfs.h"

#define DIRSEP '/'

enum FILETYPE {
    FILE_UNKN,
    FILE_REG,
    FILE_DIR,
    FILE_BLK,
    FILE_DEV,
};

enum SEEKMODE {
    SEEK_SET = 0,
    SEEK_CUR,
    SEEK_END,
};

typedef uint16_t mode_t;
#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR   3

typedef struct filehandle_t {
    size_t mountpoint; // Offset into the mountpoint table
    enum FILETYPE type;
    mode_t mode;
    void * internal_file;
    size_t size;
    size_t curr;       // Current offset
} filehandle_t;

typedef struct dirent {
    enum FILETYPE d_type;
    size_t d_size;
    uint8_t d_namlen;
    char d_name[256];
} dirent;

// Generic structure describing an FS driver
typedef struct filesystem_t {
    char name[8];
    void * (* get_fs)(part_t * p);

    filehandle_t * (* get_filehandle)(void * internal_fs, char * path, mode_t mode);
    void (* close_filehandle)(filehandle_t * f);

    size_t (* read_file)(filehandle_t * f, void * buf, size_t count);
    size_t (* write_file)(filehandle_t * f, void * buf, size_t count);

    dirent * (* read_dir)(filehandle_t * f);
} filesystem_t;

enum FILESYSTEM {
    FS_UNKN,
    FS_EXT2,
    FS_FAT32,
    FS_DEVFS,
};

typedef struct mountpoint_t {
    enum FILESYSTEM type;
    void * internal_fs; // ext2fs_t, fat32fs_t, etc.
    part_t * p;         // Might not be used on special filesystems
    char * path;        // Must end with "/"
} mountpoint_t;

typedef struct stat {
    size_t st_dev; // device, a.k.a. mountpoint #N
    uint8_t st_mode; // only FILETYPE here
    size_t st_size;
} stat;

// Needs to be in the same order as FILESYSTEM!
static const filesystem_t FILESYSTEMS[] = {
    {"unkn", NULL, NULL, NULL, NULL, NULL, NULL}, // Unknown FS

    {"ext2",
        (void* (*) (part_t * p)) get_ext2fs,
        (filehandle_t * (*) (void * internal_fs, char * path, mode_t)) ext2_getfile,
        (void (*) (filehandle_t * f)) ext2_closefile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) ext2_readfile,
        NULL,
        (dirent * (*) (filehandle_t * f)) ext2_readdir,
    },

    {"fat32",
        (void* (*) (part_t * p)) get_fat32fs,
        NULL, // TODO: Implement fat32_getfile etc.
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {"devfs",
        (void* (*) (part_t * p)) get_devfs,
        (filehandle_t * (*) (void * internal_fs, char * path, mode_t)) devfs_getfile,
        (void (*) (filehandle_t * f)) devfs_closefile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) devfs_readfile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) devfs_writefile,
        (dirent * (*) (filehandle_t * f)) devfs_readdir,
    },
};

extern mountpoint_t * mountpoints;
#define MOUNTPOINTS_N 4

void init_vfs();
filehandle_t * kopen(char * path, mode_t mode);
void kclose(filehandle_t * f);
size_t kread(filehandle_t * f, void * buf, size_t count);
size_t kwrite(filehandle_t * f, void * buf, size_t count);
dirent * kreaddir(filehandle_t * f);
void fh_to_stat(filehandle_t * in, stat * out);
