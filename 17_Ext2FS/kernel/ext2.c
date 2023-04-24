#include "ext2.h"
#include "util.h"
#include "mm.h"

ext2fs_t * get_ext2fs(part_t * p) {
    ext2fs_t * out = (ext2fs_t*)kmalloc(sizeof(ext2fs_t));

    int res = 1;
    while ((res = part_read(p, 1024, sizeof(ext2_superblock), &out->sb))) {
        if (res == 2)
            return NULL;
    }

    kputhex(out->sb.s_magic);
    kputc('\n');

    return out;
}
