#include "ata.h"
#include "util.h"

int ata_common48(Drive drive, uint64_t lba, uint16_t count) {
    bool slave = drive & 1;
    uint16_t port = ATA_IO_PORTS[drive >> 1];

    // Send IDENTIFY
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

int ata_read48(Drive drive, uint64_t lba, uint16_t count, void * buf) {
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

int ata_write48(Drive drive, uint64_t lba, uint16_t count, void * buf) {
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
