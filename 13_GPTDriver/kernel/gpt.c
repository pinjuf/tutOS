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
    for (uint8_t i = sizeof(guid.time_low) * 2 - 1; i < UINT8_MAX; i--) {
        uint8_t val = guid.time_low >> (i * 4) & 0xF;
        if (!val)
            kputc('0');
        else
            break;
    }
    kputhex(guid.time_low);

    kputc('-');

    for (uint8_t i = sizeof(guid.time_mid) * 2 - 1; i < UINT8_MAX; i--) {
        uint8_t val = guid.time_mid >> (i * 4) & 0xF;
        if (!val)
            kputc('0');
        else
            break;
    }
    kputhex(guid.time_mid);

    kputc('-');

    for (uint8_t i = sizeof(guid.time_hi_version) * 2 - 1; i < UINT8_MAX; i--) {
        uint8_t val = guid.time_hi_version >> (i * 4) & 0xF;
        if (!val)
            kputc('0');
        else
            break;
    }
    kputhex(guid.time_hi_version);

    kputc('-');

    for (uint8_t i = sizeof(guid.clock_seq_hi_variant) * 2 - 1; i < UINT8_MAX; i--) {
        uint8_t val = guid.clock_seq_hi_variant >> (i * 4) & 0xF;
        if (!val)
            kputc('0');
        else
            break;
    }
    kputhex(guid.clock_seq_hi_variant);
    for (uint8_t i = sizeof(guid.clock_seq_lo) * 2 - 1; i < UINT8_MAX; i--) {
        uint8_t val = guid.clock_seq_lo >> (i * 4) & 0xF;
        if (!val)
            kputc('0');
        else
            break;
    }
    kputhex(guid.clock_seq_lo);

    kputc('-');

    for (uint8_t i = 0; i < sizeof(guid.node); i++) {
        uint8_t c = guid.node[i];
        if (c < 16)
            kputc('0');
        kputhex(c);
    }
}
