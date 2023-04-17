#include "ata.h"
#include "util.h"
#include "mm.h"

uint8_t drive_bitmap;

int ata_common48(drive_t drive, uint64_t lba, uint16_t count) {
    bool slave = drive & 1;
    uint16_t port = ATA_IO_PORTS[drive >> 1];

    // Send IDENTIFY, just to check it's actually present & ready
    outb(port + ATA_DRIVE, 0xA0 | (slave << 4));
    outb(port + ATA_SECTOR_COUNT, 0);
    outb(port + ATA_LBA_LOW, 0);
    outb(port + ATA_LBA_MID, 0);
    outb(port + ATA_LBA_HIGH, 0);
    outb(port + ATA_COMMAND, 0xEC);
    while (true) {
        uint8_t s;
        for (size_t j = 0; j < 15; j++)
            s = inb(port + ATA_STATUS);

        if (s == 0) return 2; // Drive not responding

        if ((s & ATA_ERR) || (s & ATA_DF)) return 1;
        if ((s & ATA_DRQ) && !(s & ATA_BSY)) break;
    }

    for (int i = 0; i < 256; i++) inw(port); // Discard actual IDENTIFY data

    outb(port + ATA_DRIVE, 0x40 | (slave << 4));
    outb(port + ATA_SECTOR_COUNT, count >> 8); // Sector count high
    outb(port + ATA_LBA_LOW, lba >> 24);       // LBA 4
    outb(port + ATA_LBA_MID, lba >> 32);       // LBA 5
    outb(port + ATA_LBA_HIGH, lba >> 40);      // LBA 6
    outb(port + ATA_SECTOR_COUNT, count);      // Sector count low
    outb(port + ATA_LBA_LOW, lba);             // LBA 1
    outb(port + ATA_LBA_MID, lba >> 8);        // LBA 2
    outb(port + ATA_LBA_HIGH, lba >> 16);      // LBA 3
    
    return 0;
}

int ata_read48(drive_t drive, uint64_t lba, uint16_t count, void * buf) {
    uint16_t port = ATA_IO_PORTS[drive >> 1];

    uint8_t c = ata_common48(drive, lba, count);
    if (c != 0)
        return c;

    outb(port + ATA_COMMAND, 0x24); // ATA READ SECTORS EXT CMD

    for (size_t i = 0; i < count; i++) {
        while (true) {
            uint8_t s;
            for (size_t j = 0; j < 5; j++)
                s = inb(port + ATA_STATUS);

            if ((s & ATA_ERR) || (s & ATA_DF)) return 1;
            if ((s & ATA_DRQ) && !(s & ATA_BSY)) break;
        }

        asm volatile("rep insw" : : "d"(port), "c"(256), "D"(buf));
        buf = (void*)((uint64_t)buf + 512);
    }

    return 0;
}

int ata_write48(drive_t drive, uint64_t lba, uint16_t count, void * buf) {
    uint16_t port = ATA_IO_PORTS[drive >> 1];

    uint8_t c = ata_common48(drive, lba, count);
    if (c != 0)
        return c;

    outb(port + ATA_COMMAND, 0x34); // ATA WRITE SECTORS EXT CMD

    for (size_t i = 0; i < count; i++) {
        while (true) {
            uint8_t s;
            for (size_t j = 0; j < 15; j++)
                s = inb(port + ATA_STATUS);

            if ((s & ATA_ERR) || (s & ATA_DF)) return 1;
            if ((s & ATA_DRQ) && !(s & ATA_BSY)) break;
        }

        for (size_t j = 0; j < 256; j++)
            outw(port, ((uint16_t*)buf)[j]);
        buf = (void*)((uint64_t)buf + 512);
    }

    // Cache flush
    outb(port + ATA_COMMAND, 0xE7); // ATA FLUSH CACHE
    while (true) {
        uint8_t s;
        for (size_t j = 0; j < 15; j++)
            s = inb(port + ATA_STATUS);

        if (!(s & ATA_BSY)) break;
    }

    return 0;
}

// Like ata_read48, but with bytes for start/count instead of sectors
int drive_read(drive_t drive, size_t start, size_t count, void * buf) {
    // When calculating the amount of sectors to read, start needs to be considered too.
    // Imagine the following scenario: We are reading 512 bytes, starting at byte 256.
    // Doing just count / SECTOR_SIZE would yield 1 sector, and the %-check wouldn't engage.
    // However, such a read would require 2 sectors, as our read-area overlaps a sector border!

    uint64_t lba = start / SECTOR_SIZE;
    uint64_t sectors = (start % SECTOR_SIZE + count) / SECTOR_SIZE;
    if ((start % SECTOR_SIZE + count) % SECTOR_SIZE) sectors++;

    char * tmp_buf = (char*)kmalloc(sectors * SECTOR_SIZE);
    char * curr_tmp_buf = tmp_buf;

    size_t full_reads = sectors / ATA_MAX_SECTORS;
    for (size_t i = 0; i < full_reads; i++) {
        int result = ata_read48(drive, lba + i * ATA_MAX_SECTORS, 0, curr_tmp_buf); // 0 sectors actually means 65536 sectors
        if (result != 0)
            return result;
        curr_tmp_buf += ATA_MAX_SECTORS * SECTOR_SIZE;
    }

    int result = ata_read48(drive, lba + full_reads * ATA_MAX_SECTORS, sectors % ATA_MAX_SECTORS, curr_tmp_buf);
    if (result != 0)
        return result;

    memcpy(buf, tmp_buf + start % SECTOR_SIZE, count);
    kfree(tmp_buf);

    return 0;
}

int drive_write(drive_t drive, size_t start, size_t count, void * buf) {
    uint64_t lba = start / SECTOR_SIZE;
    uint64_t sectors = (start % SECTOR_SIZE + count) / SECTOR_SIZE;
    if ((start % SECTOR_SIZE + count) % SECTOR_SIZE) sectors++;

    char * tmp_buf = (char*)kmalloc(sectors * SECTOR_SIZE);
    char * curr_tmp_buf = tmp_buf;

    // Read the original sectors around the area we want to write to
    size_t full_rws = sectors / ATA_MAX_SECTORS;
    for (size_t i = 0; i < full_rws; i++) {
        int result = ata_read48(drive, lba + i * ATA_MAX_SECTORS, 0, curr_tmp_buf); // 0 sectors actually means 65536 sectors
        if (result != 0)
            return result;
        curr_tmp_buf += ATA_MAX_SECTORS * SECTOR_SIZE;
    }

    int result = ata_read48(drive, lba + full_rws * ATA_MAX_SECTORS, sectors % ATA_MAX_SECTORS, curr_tmp_buf);
    if (result != 0)
        return result;

    memcpy(tmp_buf + start % SECTOR_SIZE, buf, count);

    curr_tmp_buf = tmp_buf;
    for (size_t i = 0; i < full_rws; i++) {
        int result = ata_write48(drive, lba + i * ATA_MAX_SECTORS, 0, curr_tmp_buf);
        if (result != 0)
            return result;
        curr_tmp_buf += ATA_MAX_SECTORS * SECTOR_SIZE;
    }

    result = ata_write48(drive, lba + full_rws * ATA_MAX_SECTORS, sectors % ATA_MAX_SECTORS, curr_tmp_buf);
    if (result != 0)
        return result;

    kfree(tmp_buf);

    return 0;
}

void ata_checkdrives() {
    for (size_t i = 0; i < 2 * sizeof(ATA_IO_PORTS)/sizeof(ATA_IO_PORTS[0]); i++) {
        bool slave = i & 1;
        uint16_t port = ATA_IO_PORTS[i >> 1];

        outb(port + ATA_DRIVE, 0xA0 | (slave << 4));
        outb(port + ATA_SECTOR_COUNT, 0);
        outb(port + ATA_LBA_LOW, 0);
        outb(port + ATA_LBA_MID, 0);
        outb(port + ATA_LBA_HIGH, 0);
        outb(port + ATA_COMMAND, 0xEC);
        while (true) {
            uint8_t s;
            for (size_t j = 0; j < 15; j++)
                s = inb(port + ATA_STATUS);

            if (s == 0) { // Not responding
                drive_bitmap &= ~(1<<i);
                break;
            }

            if ((s & ATA_ERR) || (s & ATA_DF)) {
                drive_bitmap &= ~(1<<i);
                break;
            }

            if ((s & ATA_DRQ) && !(s & ATA_BSY)) {
                drive_bitmap |= 1<<i;
                break;
            }
        }

        for (int i = 0; i < 256; i++) inw(port); // Discard actual IDENTIFY data
    }
}

void ata_resetdrive(drive_t drive) {
    uint16_t port = ATA_CTRL_PORTS[drive >> 1];

    uint8_t status = inb(port);
    port |= 1 << 2; // SRST (Software reset, affects both drives on the bus)
    outb(port, status);

    // We should have our 5us

    status = inb(port);
    port &= ~(1 << 2);
    outb(port, status);
}
