#include "gpt.h"
#include "mm.h"
#include "util.h"

part_t * get_part(drive_t d, uint32_t n) {
    gpt_pth_t * gpt_pth = (gpt_pth_t*)kmalloc(sizeof(gpt_pth_t));

    int res = 1;
    while ((res = drive_read(d, 1*SECTOR_SIZE, sizeof(gpt_pth_t), gpt_pth))) {
        if (res == 2)
            return NULL;
    }

    // Check the signature
    if (*(uint64_t*)(&gpt_pth->sign) != GPT_SIGN) {
        kwarn(__FILE__,__func__,"no gpt signature");
    }

    if (n >= gpt_pth->gpt_entries) {
        kwarn(__FILE__,__func__,"part num out of reach");
    }

    gpt_entry_t * gpt = (gpt_entry_t*)kmalloc(gpt_pth->gpt_esize * gpt_pth->gpt_entries);

    res = 1;
    while ((res = drive_read(d, gpt_pth->gpt * SECTOR_SIZE, gpt_pth->gpt_esize * gpt_pth->gpt_entries, gpt))) {
        if (res == 2)
            return NULL;
    }

    bool zero = true;
    for (uint8_t i = 0; i < sizeof(guid_t); i++) {
        if (((char*)(&gpt[n].type_guid))[i]) {
            zero = false;
        }
    }
    if (zero) {
        kwarn(__FILE__,__func__,"part unused");
    }

    part_t * out = (part_t*)kmalloc(sizeof(part_t));
    out->d = d;
    out->n = n;
    memcpy(&out->guid, &gpt[n].uniq_guid, 16);
    memcpy(&out->type, &gpt[n].type_guid, 16);
    out->start_lba = gpt[n].start_lba;
    out->size = gpt[n].end_lba - gpt[n].start_lba + 1; // End LBA is inclusive

    kfree(gpt_pth);
    kfree(gpt);

    return out;
}

void kputguid(guid_t guid) {
    // Forgive me, but we need the leading zeroes...
    kputleadingzeroes_hex(guid.time_low, sizeof(guid.time_low)*2);
    kputhex(guid.time_low);

    kputc('-');

    kputleadingzeroes_hex(guid.time_mid, sizeof(guid.time_mid)*2);
    kputhex(guid.time_mid);

    kputc('-');

    kputleadingzeroes_hex(guid.time_hi_version, sizeof(guid.time_hi_version)*2);
    kputhex(guid.time_hi_version);

    kputc('-');

    kputleadingzeroes_hex(guid.clock_seq_hi_variant, sizeof(guid.clock_seq_hi_variant)*2);
    kputhex(guid.clock_seq_hi_variant);
    kputleadingzeroes_hex(guid.clock_seq_lo, sizeof(guid.clock_seq_lo)*2);
    kputhex(guid.clock_seq_lo);

    kputc('-');

    for (uint8_t i = 0; i < sizeof(guid.node); i++) {
        uint8_t c = guid.node[i];
        if (c < 16)
            kputc('0');
        kputhex(c);
    }
}

int part_read(part_t * p, uint64_t start, uint64_t count, void * buf) {
    if (start >= p->size * SECTOR_SIZE) {
        kwarn(__FILE__,__func__,"read out of bounds");
    }

    return drive_read(p->d, p->start_lba * SECTOR_SIZE + start, count, buf);
}

int part_write(part_t * p, uint64_t start, uint64_t count, void * buf) {
    if (start >= p->size * SECTOR_SIZE) {
        kwarn(__FILE__,__func__,"write out of bounds");
    }

    return drive_write(p->d, p->start_lba * SECTOR_SIZE + start, count, buf);
}
