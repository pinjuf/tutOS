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

    if (out->sb.s_magic != EXT2_SUPER_MAGIC)
        kwarn(__FILE__,__func__,"wrong superblock magic");

    memcpy(&out->p, p, sizeof(part_t));
    out->blocksize = 1024 << out->sb.s_log_block_size;
    out->groups_n = out->sb.s_blocks_count / out->sb.s_blocks_per_group;
    if (out->sb.s_blocks_count % out->sb.s_blocks_per_group)
        out->groups_n++;

    return out;
}
