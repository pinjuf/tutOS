#include "ext2.h"
#include "util.h"
#include "mm.h"

ext2fs_t * get_ext2fs(part_t * p) {
    ext2fs_t * out = (ext2fs_t*)kmalloc(sizeof(ext2fs_t));

    int res = 1;
    while ((res = part_read(p, 1024, sizeof(ext2_superblock_t), &out->sb))) {
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

    out->inode_size = sizeof(ext2_inode_t);
    if (out->sb.s_rev_level >= 1)
        out->inode_size = out->sb.s_inode_size;

    size_t grp_offset = out->blocksize == 1024 ? 2 : 1;
    out->grps = (ext2_blockgroupdescriptor_t *) kmalloc(sizeof(ext2_blockgroupdescriptor_t) * out->groups_n);

    res = 1;
    while ((res = part_read(p, grp_offset * out->blocksize, sizeof(ext2_blockgroupdescriptor_t) * out->groups_n, out->grps))) {
        if (res == 2)
            return NULL;
    };

    return out;
}

ext2_inode_t * get_inode(ext2fs_t * fs, uint32_t inode) {
    size_t group = (inode - 1) / fs->sb.s_inodes_per_group;
    size_t index = (inode - 1) % fs->sb.s_inodes_per_group;

    ext2_inode_t * out = (ext2_inode_t *)kmalloc(sizeof(ext2_inode_t));

    size_t offset = fs->grps[group].bg_inode_table * fs->blocksize + index * fs->inode_size;

    int res = 1;
    while ((res = part_read(&fs->p, offset, sizeof(ext2_inode_t), out))) {
        if (res == 2)
            return NULL;
    }

    return out;
}
