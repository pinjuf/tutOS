#pragma once

#include "types.h"

// BIOS Parameter Block
typedef struct fat_bpb_t {
    char jmp[3];                 // JMP SHORT 3C NOP
    char oem[8];                 // OEM Identifier
    uint16_t sectorsize;         // Bytes / sector
    uint8_t sectors_per_cluster;
    uint16_t res_sectors;        // Reserved sectors
    uint8_t fat_n;               // FAT count
    uint16_t root_entries;
    uint16_t sectors_sm;         // Small sector count
    uint8_t media_type;
    uint16_t sectors_per_fat;    // FAT12/16 only
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t sectors_lg;         // Large sector count
} __attribute__((packed)) fat_bpb_t;

// FAT32 Extended Boot record
typedef struct fat_ebr32_t {
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;    // Cluster number of the root directory
    uint16_t fsinfo_sector;   // FSInfo sector
    uint16_t backup_sector;   // Backup boot sector
    char     res[12];
    uint8_t  drive_number;
    uint8_t  nt_flags;        // Flags for WinNT
    uint8_t  sign;
    uint32_t vol_id;          // Volume ID Serial Number
    char     vol_label[11];   // Volume label
    char     sys_id[8];       // System identifier ("FAT32   ")
} __attribute__((packed)) fat_ebr32_t;

typedef struct fat_dirent83_t {
    char     name[8];
    char     ext[3];      // File extension
    uint8_t  attr;
    uint8_t  res;
    uint8_t  cr_ds;       // Creation time in tenths of seconds
    uint16_t cr_tm;       // Creation time
    uint16_t cr_dt;       // Creation date
    uint16_t last_access; // Last access date
    uint16_t cluster_hi;
    uint16_t last_mod_tm; // Last modification time
    uint16_t last_mod_dt; // Last modification date
    uint16_t cluster_lo;
    uint32_t size;        // File size
} __attribute__((packed)) fat_dirent83_t;

typedef struct fat_longname_t {
    uint8_t seq;         // Sequence number
    uint16_t name_0[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name_1[6];
    uint16_t res;
    uint16_t name_2[2];
} __attribute__((packed)) fat_longname_t;
