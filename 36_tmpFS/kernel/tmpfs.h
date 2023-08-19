#pragma once

#include "types.h"
#include "ll.h"

typedef struct tmpfs_file_t {
    char name[256];

    int type;

    union {
        struct {
            void * data;
            size_t size;
        } file;
        struct {
            ll_head * files;
        } dir;
    };
} tmpfs_file_t;

typedef struct tmpfs_t {
    tmpfs_file_t * root;
} tmpfs_t;

typedef struct tmpfs_filehandle_t {
    tmpfs_file_t * file;
} tmpfs_filehandle_t;

tmpfs_t * get_tmpfs(void * p);
int del_tmpfs(void * m);
void * tmpfs_getfile(void * mountpoint, char * path, uint16_t mode);
void tmpfs_closefile(void * f);
void * tmpfs_readdir(void * f);
int tmpfs_createfile(void * m, char * path);
int tmpfs_createdir(void * m, char * path);
size_t tmpfs_readfile(void * f, void * buf, size_t size);
size_t tmpfs_writefile(void * f, void * buf, size_t size);
int tmpfs_unlinkfile(void * m, char * path);
int tmpfs_exists(void * m, char * path);

tmpfs_file_t * tmpfs_getfiledir(ll_head * dir, char * path);
