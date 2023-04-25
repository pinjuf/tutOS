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

void ext2_read_inode(ext2fs_t * fs, ext2_inode_t * inode, void * buf) {
    void * curr = buf;
    size_t read = 0;

    const size_t ptrs_per_block = fs->blocksize/sizeof(uint32_t);

    // Read direct pointers
    for (size_t i = 0; i < 12; i++) {
        size_t to_read = fs->blocksize;
        if (inode->i_size - read < to_read)
            to_read = inode->i_size - read;

        if (inode->i_block[i]) {
            part_read(&fs->p, inode->i_block[i] * fs->blocksize, to_read, curr);
        } else {
            memset(curr, 0, to_read);
        }

        curr = (char*)curr + to_read;
        read += to_read;

        if (read >= inode->i_size) // We are done reading
            return;
    }

    // Read indirect block pointer
    if (inode->i_block[12]) {
        uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);

        part_read(&fs->p, inode->i_block[12] * fs->blocksize, fs->blocksize, indirect);

        for (size_t i = 0; i < ptrs_per_block; i++) {
            size_t to_read = fs->blocksize;
            if (inode->i_size - read < to_read)
                to_read = inode->i_size - read;

            if (indirect[i]) {
                part_read(&fs->p, indirect[i] * fs->blocksize, to_read, curr);
            } else {
                memset(curr, 0, to_read);
            }

            curr = (char*)curr + to_read;
            read += to_read;

            if (read >= inode->i_size) {
                kfree(indirect);
                return;
            }
        }

        kfree(indirect);
    } // TODO: What if the IBP is 0?

    // Read bi-indirect block pointer
    if (inode->i_block[13]) {
        uint32_t * bi_indirect = (uint32_t *) kmalloc(fs->blocksize);

        part_read(&fs->p, inode->i_block[13] * fs->blocksize, fs->blocksize, bi_indirect);

        for (size_t i = 0; i < ptrs_per_block; i++) {
            uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);

            part_read(&fs->p, bi_indirect[i] * fs->blocksize, fs->blocksize, indirect);

            for (size_t j = 0; j < ptrs_per_block; j++) {
                size_t to_read = fs->blocksize;
                if (inode->i_size - read < to_read)
                    to_read = inode->i_size - read;

                if (indirect[j]) {
                    part_read(&fs->p, indirect[j] * fs->blocksize, to_read, curr);
                } else {
                    memset(curr, 0, to_read);
                }

                curr = (char*)curr + to_read;
                read += to_read;

                if (read >= inode->i_size) {
                    kfree(bi_indirect);
                    kfree(indirect);
                    return;
                }
            }

            kfree(indirect);
        }

        kfree(bi_indirect);
    }

    // Read the tri-indirect block point
    if (inode->i_block[14]) { // TODO: Test this (with a big enough file)
        uint32_t * tri_indirect = (uint32_t *) kmalloc(fs->blocksize);

        part_read(&fs->p, inode->i_block[14] * fs->blocksize, fs->blocksize, tri_indirect);

        for (size_t i = 0; i < ptrs_per_block; i++) {
            uint32_t * bi_indirect = (uint32_t *) kmalloc(fs->blocksize);

            part_read(&fs->p, tri_indirect[i] * fs->blocksize, fs->blocksize, bi_indirect);

            for (size_t j = 0; j < ptrs_per_block; j++) {
                uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);

                part_read(&fs->p, bi_indirect[j] * fs->blocksize, fs->blocksize, indirect);

                for (size_t k = 0; k < ptrs_per_block; k++) {
                    size_t to_read = fs->blocksize;
                    if (inode->i_size - read < to_read)
                        to_read = inode->i_size - read;

                    if (indirect[k]) {
                        part_read(&fs->p, indirect[k] * fs->blocksize, to_read, curr);
                    } else {
                        memset(curr, 0, to_read);
                    }

                    curr = (char*)curr + to_read;
                    read += to_read;

                    if (read >= inode->i_size) {
                        kfree(tri_indirect);
                        kfree(bi_indirect);
                        kfree(indirect);
                        return;
                    }
                }

                kfree(indirect);
            }

            kfree(bi_indirect);
        }

        kfree(tri_indirect);
    }
}

char * ext2_lsdir(ext2fs_t * fs, ext2_inode_t * inode) {
    if (!(inode->i_mode & EXT2_S_IFDIR)) {
        kwarn(__FILE__,__func__,"non-directory inode");
    }

    // We use a bit of a special format for an ls:
    // Every entry is a NULL-terminated string,
    // and output itself is terminated by a double NULL.

    void * buf = kmalloc(inode->i_size);
    ext2_read_inode(fs, inode, buf);

    size_t read = 0;
    void * curr = buf;

    char * out = kmalloc(inode->i_size); // This leaves some overhead
    char * out_curr = out;

    while (read < inode->i_size) {
        ext2_dirent_t * entry = (ext2_dirent_t *) curr;

        memcpy(out_curr, entry->name, entry->name_len);

        curr = (char*)curr + entry->rec_len;
        read += entry->rec_len;

        out_curr += entry->name_len;
        *out_curr = 0;
        out_curr++;
    }

    *out_curr = 0;

    kfree(buf);

    return out;
}

uint32_t ext2_get_inode(ext2fs_t * fs, ext2_inode_t * inode, char * name) {
    if (!(inode->i_mode & EXT2_S_IFDIR)) {
        kwarn(__FILE__,__func__,"non-directory inode");
    }

    void * buf = kmalloc(inode->i_size);
    ext2_read_inode(fs, inode, buf);

    size_t read = 0;
    void * curr = buf;

    while (read < inode->i_size) {
        ext2_dirent_t * entry = (ext2_dirent_t *) curr;

        if (!strncmp(name, entry->name, entry->name_len)) {
            kfree(buf);
            return entry->ino;
        }

        curr = (char*)curr + entry->rec_len;
        read += entry->rec_len;
    }

    kfree(buf);

    return 0;
}
