#pragma once

#include "types.h"
#include "ata.h"

typedef struct guid_t {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_version;
    uint8_t  clock_seq_hi_variant;
    uint8_t  clock_seq_lo;
    uint8_t  node[6];
} __attribute__((packed)) guid_t;

// GPT Partition table header
typedef struct gpt_pth_t {
    char sign[8];          // GPT header ("EFI PART")
    uint32_t rev;          // GPT revision
    uint32_t size;         // Header size
    uint32_t crc32;        // Header CRC32
    uint32_t res0;         // Reserved
    uint64_t pth;          // Partition table header (this) LBA
    uint64_t pth_alt;      // Alternative PTH LBA
    uint64_t first_block;  // First usable block in a GPT entry
    uint64_t last_block;   // Last usable block
    guid_t guid;         // Disk GUID
    uint64_t gpt;          // Actual GPT LBA
    uint32_t gpt_entries;  // Number of partition entries
    uint32_t gpt_esize;    // Size of each entry
    uint32_t gpt_crc32;    // CRC32 of the partition table
} __attribute__((packed)) gpt_pth_t;

typedef struct gpt_entry_t {
    guid_t type_guid; // Type GUID
    guid_t uniq_guid; // Unique partition GUID
    uint64_t start_lba; // Start LBA
    uint64_t end_lba;   // Ending LBA
    uint64_t attr;      // Partition attributes
    uint16_t  name[36]; // SHOULD be 36 UTF-16 chars, better to use gpt_esize
} __attribute__((packed)) gpt_entry_t;

// Easy handle for a GPT partition
typedef struct part_t {
    drive_t d;
    uint16_t n;
    guid_t guid;
    guid_t type;
    uint64_t start_lba;
    uint64_t size;
} part_t;

#define GPT_SIGN 0x5452415020494645

#define GPT_WHOLEDISK UINT16_MAX

part_t * get_part(drive_t d, uint32_t n);

int part_read(part_t * p, uint64_t start, uint64_t count, void * buf);
int part_write(part_t * p, uint64_t start, uint64_t count, void * buf);

uint32_t get_part_count(drive_t d);

bool gpt_hasmagic(drive_t d);
