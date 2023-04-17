#pragma once

#include "types.h"

#define SECTOR_SIZE 0x200
#define ATA_MAX_SECTORS (1<<16)

static const uint16_t ATA_IO_PORTS[] = {
    0x1F0,
    0x170,
    0x1E8,
    0x168,
};

enum ATA_IO_REGS {
    ATA_DATA = 0,
    ATA_ERROR = 1,
    ATA_FEATURES = 1,
    ATA_SECTOR_COUNT = 2,
    ATA_LBA_LOW = 3,
    ATA_LBA_MID = 4,
    ATA_LBA_HIGH = 5,
    ATA_DRIVE = 6,
    ATA_COMMAND = 7,
    ATA_STATUS = 7,
};

// Drive / Head Register
#define ATA_SLAVE (1<<4) // Drive select
#define ATA_LBA   (1<<6) // Use LBA addressing

// Status Register
#define ATA_ERR   1      // Error
#define ATA_IDX   (1<<1) // Index (always 0)
#define ATA_CORR  (1<<2) // Corrected data (always 0)
#define ATA_DRQ   (1<<3) // Ready for PIO transfer
#define ATA_SRV   (1<<4) // Overlapped Mode Service Request
#define ATA_DF    (1<<5) // Drive Fault Error
#define ATA_RDY   (1<<6) // Drive Ready
#define ATA_BSY   (1<<7) // Busy

typedef uint8_t drive_t;

extern uint8_t drive_bitmap;

int ata_read48(drive_t drive, uint64_t lba, uint16_t count, void * buf);
int ata_write48(drive_t drive, uint64_t lba, uint16_t count, void * buf);

int drive_read(drive_t drive, size_t start, size_t count, void * buf);
int drive_write(drive_t drive, size_t start, size_t count, void * buf);

void ata_checkdrives();
