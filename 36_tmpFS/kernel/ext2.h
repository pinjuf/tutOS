#pragma once

#include "gpt.h"
#include "types.h"

// See https://www.nongnu.org/ext2-doc/ext2.html for standard & docs

// The superblock contains infos about the entire FS
typedef struct ext2_superblock_t {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    guid_t   s_uuid;
} __attribute__((packed)) ext2_superblock_t;

// s_magic
#define EXT2_SUPER_MAGIC 0xEF53

// s_state
#define EXT2_VALID_FS 1
#define EXT2_ERROR_FS 2

// s_errors
#define EXT2_ERRORS_CONTINUE 1
#define EXT2_ERRORS_RO       2
#define EXT2_ERRORS_PANIC    3

// s_creator_os
#define EXT2_OS_LINUX   0
#define EXT2_OS_HURD    1
#define EXT2_OS_MASIX   2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES   4

// s_rev_level
#define EXT2_GOOD_OLD_REV 0
#define EXT2_DYNAMIC_REV  1

typedef struct ext2_blockgroupdescriptor_t {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
} __attribute__((packed)) ext2_blockgroupdescriptor_t;

typedef struct ext2_inode_t {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
} __attribute__((packed)) ext2_inode_t;

// reserved inodes
#define EXT2_BAD_INO         1
#define EXT2_ROOT_INO        2
#define EXT2_ACL_IDX_INO     3
#define EXT2_ACL_DATA_INO    4
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO   6

// i_mode
#define EXT2_S_IFMSK  0xF000

#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK  0xA000
#define EXT2_S_IFREG  0x8000
#define EXT2_S_IFBLK  0x6000
#define EXT2_S_IFDIR  0x4000
#define EXT2_S_IFCHR  0x2000
#define EXT2_S_IFIFO  0x1000
#define EXT2_S_ISUID  0x0800
#define EXT2_S_ISGID  0x0400
#define EXT2_S_ISVTX  0x0200
#define EXT2_S_IRUSR  0x0100
#define EXT2_S_IWUSR  0x0080
#define EXT2_S_IXUSR  0x0040
#define EXT2_S_IRGRP  0x0020
#define EXT2_S_IWGRP  0x0010
#define EXT2_S_IXGRP  0x0008
#define EXT2_S_IROTH  0x0004
#define EXT2_S_IWOTH  0x0002
#define EXT2_S_IXOTH  0x0001

typedef struct ext2_dirent {
    uint32_t ino;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
} __attribute__((packed)) ext2_dirent;

#define EXT2_FT_UNKNOWN  0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR      2
#define EXT2_FT_CHRDEV   3
#define EXT2_FT_BLKDEV   4
#define EXT2_FT_FIFO     5
#define EXT2_FT_SOCK     6
#define EXT2_FT_SYMLINK  7

// A simple handle for an entire ext2fs
typedef struct ext2fs_t {
    ext2_superblock_t sb;
    size_t blocksize;
    size_t groups_n;
    size_t inode_size;
    ext2_blockgroupdescriptor_t * grps;
} ext2fs_t;

typedef struct ext2fs_file_t {
    ext2_inode_t * inode;
    void * cache; // Caches a block of the file
    uint32_t cache_index; // Index of the cached block
} ext2fs_file_t;

ext2fs_t * get_ext2fs(void * f);
ext2_inode_t * ext2_get_inode(void *, uint32_t inode);
void ext2_read_inode(void *, ext2_inode_t * inode, void * buf);
uint32_t ext2_get_inode_by_name(void *, ext2_inode_t * inode, char * name);
void * ext2_getfile(void * mn, char * path, uint16_t);
void ext2_closefile(void * f);
size_t ext2_readfile(void * f, void * buf, size_t count);
void * ext2_readdir(void * f);
int del_ext2fs(void * m);
void ext2_read_inodeblock(void * mn, ext2_inode_t * inode, void * buf, size_t n);
int ext2_exists(void * m, char * path);
