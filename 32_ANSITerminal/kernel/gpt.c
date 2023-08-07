#include "gpt.h"
#include "mm.h"
#include "util.h"

bool gpt_hasmagic(drive_t d) {
    gpt_pth_t * gpt_pth = (gpt_pth_t*)kmalloc(sizeof(gpt_pth_t));
    drive_read(d, 1*SECTOR_SIZE, sizeof(gpt_pth_t), gpt_pth);
    kfree(gpt_pth);

    return *(uint64_t*)(&gpt_pth->sign) == GPT_SIGN;
}

part_t * get_part(drive_t d, uint32_t n) {
    gpt_pth_t * gpt_pth = (gpt_pth_t*)kmalloc(sizeof(gpt_pth_t));

    // GPT_WHOLEDISK means to use the whole disk
    if (n == GPT_WHOLEDISK) {
        part_t * out = (part_t*)kmalloc(sizeof(part_t));
        out->d = d;
        out->n = n;
        out->start_lba = 0;
        out->size = UINT64_MAX;

        kfree(gpt_pth);

        return out;
    }

    drive_read(d, 1*SECTOR_SIZE, sizeof(gpt_pth_t), gpt_pth);

    // Check the signature
    if (*(uint64_t*)(&gpt_pth->sign) != GPT_SIGN) {
        kwarn(__FILE__,__func__,"no gpt signature");
    }

    if (n >= gpt_pth->gpt_entries) {
        kwarn(__FILE__,__func__,"part num out of reach");
    }

    gpt_entry_t * gpt = (gpt_entry_t*)kmalloc(gpt_pth->gpt_esize);

    drive_read(d, gpt_pth->gpt * SECTOR_SIZE + n * gpt_pth->gpt_esize, gpt_pth->gpt_esize, gpt);

    bool zero = true;
    for (uint8_t i = 0; i < sizeof(guid_t); i++) {
        if (((char*)(&gpt->type_guid))[i]) {
            zero = false;
        }
    }
    if (zero) {
        kwarn(__FILE__,__func__,"part unused");
    }

    part_t * out = (part_t*)kmalloc(sizeof(part_t));
    out->d = d;
    out->n = n;
    memcpy(&out->guid, &gpt->uniq_guid, 16);
    memcpy(&out->type, &gpt->type_guid, 16);
    out->start_lba = gpt->start_lba;
    out->size = gpt->end_lba - gpt->start_lba + 1; // End LBA is inclusive

    kfree(gpt_pth);
    kfree(gpt);

    return out;
}

uint32_t get_part_count(drive_t d) {
    gpt_pth_t * gpt_pth = (gpt_pth_t*)kmalloc(sizeof(gpt_pth_t));

    drive_read(d, 1*SECTOR_SIZE, sizeof(gpt_pth_t), gpt_pth);

    // Check the signature
    if (*(uint64_t*)(&gpt_pth->sign) != GPT_SIGN) {
        kwarn(__FILE__,__func__,"no gpt signature");
    }

    gpt_entry_t * gpt = (gpt_entry_t*)kmalloc(gpt_pth->gpt_esize * gpt_pth->gpt_entries);

    drive_read(d, gpt_pth->gpt * SECTOR_SIZE, gpt_pth->gpt_esize * gpt_pth->gpt_entries, gpt);

    uint32_t out = 0;
    for (size_t i = 0; i < gpt_pth->gpt_entries; i++) {
        // We can't trust sizeof() because of gpt_esize
        gpt_entry_t * g = (void*)((size_t)gpt + gpt_pth->gpt_esize * i);

        bool zero = true;
        for (uint8_t i = 0; i < sizeof(guid_t); i++) {
            if (((char*)(&g->type_guid))[i]) {
                zero = false;
            }
        }

        if (!zero)
            out++;
    }

    kfree(gpt_pth);
    kfree(gpt);

    return out;
}

int part_read(part_t * p, uint64_t start, uint64_t count, void * buf) {
    if (start >= (p->size * SECTOR_SIZE)) {
        kwarn(__FILE__,__func__,"read out of bounds");
        return 1;
    }

    return drive_read(p->d, p->start_lba * SECTOR_SIZE + start, count, buf);
}

int part_write(part_t * p, uint64_t start, uint64_t count, void * buf) {
    if (start >= (p->size * SECTOR_SIZE)) {
        kwarn(__FILE__,__func__,"write out of bounds");
        return 1;
    }

    return drive_write(p->d, p->start_lba * SECTOR_SIZE + start, count, buf);
}
