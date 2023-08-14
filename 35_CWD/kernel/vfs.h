#pragma once

#include "types.h"
#include "gpt.h"
#include "fat.h"
#include "ext2.h"
#include "devfs.h"
#include "../lib/unistd.h"

#define DIRSEP '/'

typedef struct filehandle_t {
    size_t mountpoint; // Offset into the mountpoint table
    enum FILETYPE type;
    mode_t mode;
    void * internal_file;
    size_t size;
    size_t curr;       // Current offset
    size_t fd_refs;    // Number of file descriptors connected to this
} filehandle_t;

enum FILESYSTEM {
    FS_UNKN,
    FS_EXT2,
    FS_FAT32,
    FS_DEVFS,
};

typedef struct mountpoint_t {
    enum FILESYSTEM type;
    char * filepath;     // File that is mounted
    filehandle_t * file;
    void * internal_fs;  // ext2fs_t, fat32fs_t, etc.
    char * path;         // Must end with "/"
} mountpoint_t;

// Generic structure describing an FS driver
typedef struct filesystem_t {
    char name[8];

    uint16_t default_rw; // How the mountfile should be opened

    void * (* get_fs)(filehandle_t * f);
    int    (* del_fs)(mountpoint_t * mountpoint);

    filehandle_t * (* get_filehandle)(void * mountpoint, char * path, mode_t mode);
    void (* close_filehandle)(filehandle_t * f);

    size_t (* read_file)(filehandle_t * f, void * buf, size_t count);
    size_t (* write_file)(filehandle_t * f, void * buf, size_t count);

    dirent * (* read_dir)(filehandle_t * f);
} filesystem_t;

// Needs to be in the same order as FILESYSTEM!
static const filesystem_t FILESYSTEMS[] = {
    {"unkn", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL}, // Unknown FS

    {"ext2",
        O_RDONLY,
        (void* (*) (filehandle_t * f)) get_ext2fs,
        (int (*) (mountpoint_t * m)) del_ext2fs,
        (filehandle_t * (*) (void * internal_fs, char * path, mode_t)) ext2_getfile,
        (void (*) (filehandle_t * f)) ext2_closefile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) ext2_readfile,
        NULL,
        (dirent * (*) (filehandle_t * f)) ext2_readdir,
    },

    {"fat32",
        0,
        NULL, // TODO: Implement perhaps
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    },

    {"devfs",
        O_RDWR,
        (void* (*) (filehandle_t * fh)) get_devfs,
        (int (*) (mountpoint_t * m)) del_devfs,
        (filehandle_t * (*) (void * internal_fs, char * path, mode_t)) devfs_getfile,
        (void (*) (filehandle_t * f)) devfs_closefile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) devfs_readfile,
        (size_t (*) (filehandle_t * f, void * buf, size_t count)) devfs_writefile,
        (dirent * (*) (filehandle_t * f)) devfs_readdir,
    },
};

extern mountpoint_t * mountpoints;
#define MOUNTPOINTS_N 16

void init_vfs();
int _mount(char * filepath, char * mountpoint, enum FILESYSTEM type);
int mount(char * filepath, char * mountpoint, char * type);

filehandle_t * kopen(char * path, mode_t mode);
void kclose(filehandle_t * f);
size_t kread(filehandle_t * f, void * buf, size_t count);
size_t kreadat(filehandle_t * f, size_t off, void * buf, size_t count);
size_t kwrite(filehandle_t * f, void * buf, size_t count);
size_t kwriteat(filehandle_t * f, size_t off, void * buf, size_t count);
dirent * kreaddir(filehandle_t * f);
void fh_to_stat(filehandle_t * in, stat * out);

int remove_pathddots(char * path);
int remove_pathdseps(char * path);
int remove_pathdots(char * path);
int clean_path(char * path);

int unmount(char * mountpoint);
