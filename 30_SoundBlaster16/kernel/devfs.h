#pragma once

#include "gpt.h"
#include "ll.h"

typedef uint32_t devfs_id_t;

typedef struct devfs_t {
    ll_head * devs;
    devfs_id_t id_counter;
} devfs_t;

typedef struct devfs_file_t {
    devfs_id_t id;
} devfs_file_t;

typedef struct devfs_dev_t {
    devfs_id_t id;
    char name[256];
    int type; // Fuck you, dear C standard, for not letting me use enum FILETYPE
    size_t size;
    uint16_t avl_modes; // Available modes

    size_t (* read)    (void * f, void * buf, size_t count);
    size_t (* write)   (void * f, void * buf, size_t count);
    void * (* readdir) (void * f);

    union {
        part_t p; // Used when the device is a HDD
    } spec; // Specific fields
} devfs_dev_t;

devfs_t * get_devfs(void * p);

devfs_id_t devfs_register_dev(devfs_t * fs, devfs_dev_t * dev);
void devfs_unregister_dev(devfs_t * fs, devfs_id_t dev);

void * devfs_getfile(void *, char * path, uint16_t);
void devfs_closefile(void * f);
size_t devfs_readfile(void * f, void * buf, size_t count);
size_t devfs_writefile(void * f, void * buf, size_t count);
void * devfs_readdir(void * f);

void * devfs_readdir_rootdir(void * f);
size_t devfs_read_tty(void * f, void * buf, size_t count);
size_t devfs_write_tty(void * f, void * buf, size_t count);
size_t devfs_read_vesa(void * f, void * buf, size_t count);
size_t devfs_write_vesa(void * f, void * buf, size_t count);
size_t devfs_read_mem(void * f, void * buf, size_t count);
size_t devfs_write_mem(void * f, void * buf, size_t count);
size_t devfs_read_hdd(void * f, void * buf, size_t count);
size_t devfs_write_hdd(void * f, void * buf, size_t count);
size_t devfs_read_pit0(void * f, void * buf, size_t count);
size_t devfs_read_dsp(void * f, void * buf, size_t count);
size_t devfs_write_dsp(void * f, void * buf, size_t count);
