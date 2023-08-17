#include "ext2.h"
#include "util.h"
#include "mm.h"
#include "vfs.h"

ext2fs_t * get_ext2fs(void * f) {
    filehandle_t * fh = f;

    ext2fs_t * out = (ext2fs_t*)kmalloc(sizeof(ext2fs_t));

    kreadat(fh, 1024, &out->sb, sizeof(ext2_superblock_t));

    if (out->sb.s_magic != EXT2_SUPER_MAGIC)
        kwarn(__FILE__,__func__,"wrong superblock magic");

    out->blocksize = 1024 << out->sb.s_log_block_size;
    out->groups_n = out->sb.s_blocks_count / out->sb.s_blocks_per_group;
    if (out->sb.s_blocks_count % out->sb.s_blocks_per_group)
        out->groups_n++;

    out->inode_size = sizeof(ext2_inode_t);
    if (out->sb.s_rev_level >= 1)
        out->inode_size = out->sb.s_inode_size;

    size_t grp_offset = out->blocksize == 1024 ? 2 : 1;
    out->grps = (ext2_blockgroupdescriptor_t *) kmalloc(sizeof(ext2_blockgroupdescriptor_t) * out->groups_n);

    kreadat(fh, grp_offset * out->blocksize, out->grps, sizeof(ext2_blockgroupdescriptor_t) * out->groups_n);

    return out;
}

int del_ext2fs(void * m) {
    mountpoint_t * mnt = m;
    ext2fs_t * fs = mnt->internal_fs;

    kfree(fs->grps);
    kfree(fs);

    return 0;
}

ext2_inode_t * ext2_get_inode(void * mn, uint32_t inode) {
    mountpoint_t * mnt = mn;
    ext2fs_t * fs = mnt->internal_fs;

    size_t group = (inode - 1) / fs->sb.s_inodes_per_group;
    size_t index = (inode - 1) % fs->sb.s_inodes_per_group;

    ext2_inode_t * out = (ext2_inode_t *)kmalloc(sizeof(ext2_inode_t));

    size_t offset = fs->grps[group].bg_inode_table * fs->blocksize + index * fs->inode_size;

    kreadat(mnt->file, offset, out, sizeof(ext2_inode_t));

    return out;
}

void ext2_read_inode(void * mn, ext2_inode_t * inode, void * buf) {
    mountpoint_t * mnt = mn;
    ext2fs_t * fs = mnt->internal_fs;

    void * curr = buf;
    size_t read = 0;

    const size_t ptrs_per_block = fs->blocksize/sizeof(uint32_t);

    // Read direct pointers
    for (size_t i = 0; i < 12; i++) {
        size_t to_read = fs->blocksize;
        if (inode->i_size - read < to_read)
            to_read = inode->i_size - read;

        if (inode->i_block[i]) {
            kreadat(mnt->file, inode->i_block[i] * fs->blocksize, curr, to_read);
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

        kreadat(mnt->file, inode->i_block[12] * fs->blocksize, indirect, fs->blocksize);

        for (size_t i = 0; i < ptrs_per_block; i++) {
            size_t to_read = fs->blocksize;
            if (inode->i_size - read < to_read)
                to_read = inode->i_size - read;

            if (indirect[i]) {
                kreadat(mnt->file, indirect[i] * fs->blocksize, curr, to_read);
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

        kreadat(mnt->file, inode->i_block[13] * fs->blocksize, bi_indirect, fs->blocksize);

        for (size_t i = 0; i < ptrs_per_block; i++) {
            uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);

            kreadat(mnt->file, bi_indirect[i] * fs->blocksize, indirect, fs->blocksize);

            for (size_t j = 0; j < ptrs_per_block; j++) {
                size_t to_read = fs->blocksize;
                if (inode->i_size - read < to_read)
                    to_read = inode->i_size - read;

                if (indirect[j]) {
                    kreadat(mnt->file, indirect[j] * fs->blocksize, curr, to_read);
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

        kreadat(mnt->file, inode->i_block[14] * fs->blocksize, tri_indirect, fs->blocksize);

        for (size_t i = 0; i < ptrs_per_block; i++) {
            uint32_t * bi_indirect = (uint32_t *) kmalloc(fs->blocksize);

            kreadat(mnt->file, tri_indirect[i] * fs->blocksize, bi_indirect, fs->blocksize);

            for (size_t j = 0; j < ptrs_per_block; j++) {
                uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);

                kreadat(mnt->file, bi_indirect[j] * fs->blocksize, indirect, fs->blocksize);

                for (size_t k = 0; k < ptrs_per_block; k++) {
                    size_t to_read = fs->blocksize;
                    if (inode->i_size - read < to_read)
                        to_read = inode->i_size - read;

                    if (indirect[k]) {
                        kreadat(mnt->file, indirect[k] * fs->blocksize, curr, to_read);
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

void ext2_read_inodeblock(void * mn, ext2_inode_t * inode, void * buf, size_t n) {
    mountpoint_t * mnt = mn;
    ext2fs_t * fs      = mnt->internal_fs;

    const size_t ptrs_per_block = fs->blocksize/sizeof(uint32_t);

    if (n >= (inode->i_size + fs->blocksize - 1) / fs->blocksize) {
        kwarn(__FILE__,__func__,"block out of range");
        return;
    }

    if (n < 12) {
        if (!inode->i_block[n]) {
            memset(buf, 0, fs->blocksize);
            return;
        }

        kreadat(mnt->file, inode->i_block[n] * fs->blocksize, buf, fs->blocksize);

    } else if (n < (12 \
                  + ptrs_per_block)) {

        uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);
        kreadat(mnt->file, inode->i_block[12] * fs->blocksize, indirect, fs->blocksize);

        const uint32_t offset = n - 12;

        if (!indirect[offset]) {
            memset(buf, 0, fs->blocksize);
            kfree(indirect);
            return;
        }

        kreadat(mnt->file, indirect[offset] * fs->blocksize, buf, fs->blocksize);

        kfree(indirect);

    } else if (n < (12 \
                  + ptrs_per_block \
                  + ptrs_per_block * ptrs_per_block)) {

        uint32_t * bi_indirect = (uint32_t *) kmalloc(fs->blocksize);
        kreadat(mnt->file, inode->i_block[13] * fs->blocksize, bi_indirect, fs->blocksize);

        const uint32_t bi_offset = (n - 12 - ptrs_per_block)/ptrs_per_block;

        uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);
        kreadat(mnt->file, bi_indirect[bi_offset] * fs->blocksize, indirect, fs->blocksize);

        const uint32_t offset = (n - 12 - ptrs_per_block) % ptrs_per_block;

        if (!indirect[offset]) {
            memset(buf, 0, fs->blocksize);
            kfree(indirect);
            kfree(bi_indirect);
            return;
        }

        kreadat(mnt->file, indirect[offset] * fs->blocksize, buf, fs->blocksize);

        kfree(indirect);
        kfree(bi_indirect);
    } else if (n < (12 \
                  + ptrs_per_block \
                  + ptrs_per_block * ptrs_per_block \
                  + ptrs_per_block * ptrs_per_block * ptrs_per_block)) {

        uint32_t * tri_indirect = (uint32_t *) kmalloc(fs->blocksize);
        kreadat(mnt->file, inode->i_block[14] * fs->blocksize, tri_indirect, fs->blocksize);

        //const uint32_t tri_offset = (n - 12 - ptrs_per_block - ptrs_per_block * ptrs_per_block)/(ptrs_per_block * ptrs_per_block);

        uint32_t tri_offset = n - 12 - ptrs_per_block - ptrs_per_block * ptrs_per_block;
        tri_offset /= ptrs_per_block * ptrs_per_block;

        uint32_t * bi_indirect = (uint32_t *) kmalloc(fs->blocksize);
        kreadat(mnt->file, tri_indirect[tri_offset] * fs->blocksize, bi_indirect, fs->blocksize);

        uint32_t bi_offset = n - 12 - ptrs_per_block - ptrs_per_block * ptrs_per_block;
        bi_offset         -= tri_offset * ptrs_per_block * ptrs_per_block;
        bi_offset         /= ptrs_per_block;

        uint32_t * indirect = (uint32_t *) kmalloc(fs->blocksize);
        kreadat(mnt->file, bi_indirect[bi_offset] * fs->blocksize, indirect, fs->blocksize);

        uint32_t offset = n - 12 - ptrs_per_block - ptrs_per_block * ptrs_per_block;
        offset         -= tri_offset * ptrs_per_block * ptrs_per_block;
        offset         -= bi_offset * ptrs_per_block;

        if (!indirect[offset]) {
            memset(buf, 0, fs->blocksize);
            kfree(indirect);
            kfree(bi_indirect);
            kfree(tri_indirect);
            return;
        }

        kreadat(mnt->file, indirect[offset] * fs->blocksize, buf, fs->blocksize);

        kfree(indirect);
        kfree(bi_indirect);
        kfree(tri_indirect);
    } else {
        kwarn(__FILE__,__func__,"block out of range");
    }
}

uint32_t ext2_get_inode_by_name(void * mn, ext2_inode_t * inode, char * name) {
    mountpoint_t * mnt = mn;

    if (!(inode->i_mode & EXT2_S_IFDIR)) {
        kwarn(__FILE__,__func__,"non-directory inode");
    }

    void * buf = kmalloc(inode->i_size);
    ext2_read_inode(mnt, inode, buf);

    size_t read = 0;
    void * curr = buf;

    while (read < inode->i_size) {
        ext2_dirent * entry = (ext2_dirent *) curr;

        if (!strncmp(name, entry->name, entry->name_len) && entry->name_len == strlen(name)) {
            kfree(buf);
            return entry->ino;
        }

        curr = (char*)curr + entry->rec_len;
        read += entry->rec_len;
    }

    kfree(buf);

    return 0;
}

void * ext2_getfile(void * mn, char * path, uint16_t m) {
    mountpoint_t * mnt = mn;
    mode_t mode = m; // compiler's fault
    char token[256];

    if (mode & O_WRONLY) {
        kwarn(__FILE__,__func__,"write not implemented");
        return NULL;
    }

    ext2_inode_t * curr = ext2_get_inode(mnt, EXT2_ROOT_INO);

    // Courtesy of ChatGPT cause I am tired
    char * p = path;
    while (*p) {

        while (*p == DIRSEP)
            p++;

        char * tok = p;
        while (*p && *p != DIRSEP) {
            p++;
        }

        if (tok == p)
            continue;

        memset(token, 0, 256);
        memcpy(token, tok, p-tok);

        uint32_t next = ext2_get_inode_by_name(mnt, curr, token);
        kfree(curr);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return NULL;
        }

        curr = ext2_get_inode(mnt, next);
    }

    filehandle_t * out = kmalloc(sizeof(filehandle_t));

    ext2fs_file_t * intern_out = kmalloc(sizeof(ext2fs_file_t));

    intern_out->inode = curr;
    intern_out->cache = NULL;

    out->internal_file = intern_out;
    out->curr = 0;
    out->size = curr->i_size;

    switch (curr->i_mode & EXT2_S_IFMSK) {
        case EXT2_S_IFREG:
            out->type = FILE_REG;
            break;
        case EXT2_S_IFDIR:
            out->type = FILE_DIR;
            break;
        default:
            out->type = FILE_UNKN;
            break;
    }

    return out;
}

void ext2_closefile(void * f) {
    filehandle_t * fh = f;
    ext2fs_file_t * intern = fh->internal_file;

    kfree(intern->inode);
    if (intern->cache)
        kfree(intern->cache);

    kfree(intern);
    kfree(fh);
}

size_t ext2_readfile(void * f, void * buf, size_t count) {
    filehandle_t * fh      = f;
    ext2fs_file_t * intern = fh->internal_file;
    mountpoint_t * mnt     = &mountpoints[fh->mountpoint];
    ext2fs_t * fs          = mnt->internal_fs;

    if (fh->type == FILE_DIR) {
        kwarn(__FILE__,__func__,"trying to file-read directory");
    }

    size_t to_read = count;
    if (fh->curr + to_read > fh->size) {
        to_read = fh->size - fh->curr;
    }

    // The file is to be read block by block
    size_t read = 0;

    // Generic case:
    //
    //       Block 0           Block 1           Block 2           Block 3
    // |-----------------|-----------------|-----------------|-----------------|
    //         ^ start (fh->curr)                end (fh->curr + to_read) ^
    //         |<----------------- to_read ----------------------------->|
    //         |<------->||<-------------------------------->|<--------->|
    //  start overlap                   main align               end overlap

    // Special case:
    //
    //       Block 0           Block 1           Block 2           Block 3
    // |-----------------|-----------------|-----------------|-----------------|
    //         ^      ^
    //         |<--->|

    // Calculate the total number of blocks to read
    uint32_t start_block = fh->curr / fs->blocksize;
    uint32_t end_block   = (fh->curr + to_read) / fs->blocksize;
    if ((fh->curr + to_read) % fs->blocksize)
        end_block++;

    if (!intern->cache) {
        intern->cache = kmalloc(fs->blocksize);
        intern->cache_index = UINT32_MAX;
    }

    for (uint32_t i = 0; i < end_block - start_block; i++) {
        if (intern->cache_index != start_block + i) {
            ext2_read_inodeblock(mnt, intern->inode, intern->cache, start_block + i);
            intern->cache_index = start_block + i;
        }

        // From where to where are we transfering in this block?
        size_t start = fh->curr % fs->blocksize;
        size_t end   = MIN(start + to_read, fs->blocksize);
        size_t sz    = end - start;

        memcpy((void*)((uint64_t)buf + read), (void*)((uint64_t)intern->cache + start), sz);

        read += sz;
        to_read -= sz;
        fh->curr += sz;
    }

    return to_read;
}

void * ext2_readdir(void * f) {
    filehandle_t * fh = f;
    ext2fs_file_t * intern = fh->internal_file;

    if (fh->type != FILE_DIR) {
        kwarn(__FILE__,__func__,"trying to dir-read file");
    }

    if (!intern->cache) {
        intern->cache = kmalloc(fh->size);

        ext2_read_inode(&mountpoints[fh->mountpoint], intern->inode, intern->cache);
    }

    ext2_dirent * entry = (ext2_dirent*) ((size_t)intern->cache + fh->curr);

    if (entry->ino == 0) // Technically means unused entry, but generally indicates End Of Directory
        return NULL;

    if (fh->curr >= fh->size) // End of directory
        return NULL;

    dirent * out = kcalloc(sizeof(dirent));

    switch (entry->file_type) {
        case EXT2_FT_REG_FILE:
            out->d_type = FILE_REG;
            break;
        case EXT2_FT_DIR:
            out->d_type = FILE_DIR;
            break;
        default:
            out->d_type = FILE_UNKN;
            break;
    }

    memcpy(out->d_name, entry->name, entry->name_len);
    out->d_namlen = strlen(out->d_name);

    ext2_inode_t * ino = ext2_get_inode(&mountpoints[fh->mountpoint], entry->ino);
    out->d_size = ino->i_size;
    kfree(ino);

    fh->curr += entry->rec_len;

    return out;
}
