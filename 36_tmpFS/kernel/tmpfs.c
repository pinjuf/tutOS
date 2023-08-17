#include "tmpfs.h"
#include "util.h"
#include "mm.h"
#include "vfs.h"

tmpfs_t * get_tmpfs(void * p) {
    (void) p; // tmpFS isn't based off of any file

    tmpfs_t * out = kmalloc(sizeof(tmpfs_t));

    tmpfs_file_t * root = kmalloc(sizeof(tmpfs_file_t));
    root->type = FILE_DIR;
    strcpy(root->name, "");
    root->dir.files = create_ll(sizeof(tmpfs_file_t));

    out->root = root;

    return out;
}

int del_tmpfs(void * m) {
    mountpoint_t * mnt = (mountpoint_t *) m;
    tmpfs_t * tmpfs = (tmpfs_t *) mnt->internal_fs;

    // TODO: Delete recursively!

    kfree(tmpfs);

    return 0;
}

void * tmpfs_getfile(void * mountpoint, char * path, uint16_t mode) {
    (void) mode; // tmpfs doesn't care about mode

    char token[256];

    mountpoint_t * mnt = (mountpoint_t *) mountpoint;
    tmpfs_t * tmpfs = (tmpfs_t *) mnt->internal_fs;

    tmpfs_file_t * file = tmpfs->root;
    ll_head * curr = tmpfs->root->dir.files;

    char * p = path;
    while (*p) {

        while (*p == DIRSEP)
            p++;

        char * tok = p;
        while (*p && *p != DIRSEP)
            p++;

        if (tok == p)
            continue;

        memset(token, 0, 256);
        memcpy(token, tok, p - tok);

        tmpfs_file_t * next = tmpfs_getfiledir(curr, token);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return NULL;
        }

        if (!*p) {
            file = next;
            break;
        }

        if (next->type != FILE_DIR) {
            kwarn(__FILE__,__func__,"not a directory");
            return NULL;
        }

        curr = next->dir.files;
    }

    filehandle_t * fh = kmalloc(sizeof(filehandle_t));
    tmpfs_filehandle_t * intern = fh->internal_file = kmalloc(sizeof(tmpfs_filehandle_t));

    intern->file = file;
    fh->curr = 0;
    fh->type = file->type;

    switch (fh->type) {
        case FILE_REG:
            fh->size = file->file.size;
            break;
        case FILE_DIR:
            fh->size = ll_len(file->dir.files);
            break;
        default:
            kwarn(__FILE__,__func__,"unknown file type (corrupt fs?)");
            return NULL;
    }

    return fh;
}

tmpfs_file_t * tmpfs_getfiledir(ll_head * dir, char * path) {
    tmpfs_file_t * out = NULL;

    for (size_t i = 0; i < ll_len(dir); i++) {
        tmpfs_file_t * file = (tmpfs_file_t *) ll_get(dir, i);

        if (strcmp(file->name, path) == 0) {
            out = file;
            break;
        }
    }

    return out;
}

void tmpfs_closefile(void * f) {
    filehandle_t * fh = f;
    tmpfs_filehandle_t * tmpfs_fh = fh->internal_file;

    kfree(tmpfs_fh);
    kfree(fh);
}

void * tmpfs_readdir(void * f) {
    filehandle_t * fh = f;
    tmpfs_filehandle_t * tmpfs_fh = fh->internal_file;

    if (fh->type != FILE_DIR) {
        kwarn(__FILE__,__func__,"not a directory");
        return NULL;
    }

    if (fh->curr >= ll_len(tmpfs_fh->file->dir.files))
        return NULL;

    tmpfs_file_t * file = (tmpfs_file_t *) ll_get(tmpfs_fh->file->dir.files, fh->curr);
    fh->curr++;

    dirent * out = kmalloc(sizeof(dirent));

    strcpy(out->d_name, file->name);
    out->d_type = file->type;
    out->d_namlen = strlen(file->name);
    switch (file->type) {
        case FILE_REG:
            out->d_size = file->file.size;
            break;
        case FILE_DIR:
            out->d_size = ll_len(file->dir.files);
            break;
        default:
            kwarn(__FILE__,__func__,"unknown file type (corrupt fs?)");
            return NULL;
    }

    return out;
}

int tmpfs_createfile(void * m, char * path) {
    mountpoint_t * mnt = m;
    tmpfs_t * tmpfs = mnt->internal_fs;

    char filename[256];
    char * o = path + strlen(path) - 1;
    while (*o == DIRSEP && o > path)
        o--;
    memcpy(filename, path, o - path + 1);
    filename[o - path + 1] = 0;

    *o = 0;

    char token[256];

    // Get the dir we're creating the file in
    ll_head * dir = tmpfs->root->dir.files;

    char * p = path;
    while (*p) {
        while (*p == DIRSEP)
            p++;

        char * tok = p;
        while (*p && *p != DIRSEP)
            p++;

        if (tok == p)
            continue;

        if (!*p)
            break;

        memset(token, 0, 256);
        memcpy(token, tok, p - tok);

        tmpfs_file_t * next = tmpfs_getfiledir(dir, token);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return -1;
        }

        if (next->type != FILE_DIR) {
            kwarn(__FILE__,__func__,"not a directory");
            return -1;
        }

        dir = next->dir.files;
    }

    if (tmpfs_getfiledir(dir, filename)) {
        kwarn(__FILE__,__func__,"file already exists");
        return -1;
    }

    tmpfs_file_t * file = ll_push(dir);

    file->type = FILE_REG;
    strcpy(file->name, filename);
    file->file.size = 0;
    file->file.data = NULL;

    return 0;
}

size_t tmpfs_readfile(void * f, void * buf, size_t size) {
    filehandle_t * fh = f;
    tmpfs_filehandle_t * tmpfs_fh = fh->internal_file;
    tmpfs_file_t * file = tmpfs_fh->file;

    size_t to_read = size;
    if (to_read > file->file.size - fh->curr)
        to_read = file->file.size - fh->curr;

    memcpy(buf, file->file.data + fh->curr, to_read);

    fh->curr += to_read;

    return to_read;
}

size_t tmpfs_writefile(void * f, void * buf, size_t size) {
    filehandle_t * fh = f;
    tmpfs_filehandle_t * tmpfs_fh = fh->internal_file;
    tmpfs_file_t * file = tmpfs_fh->file;

    size_t new_size = fh->curr + size;

    if (new_size > file->file.size) {
        file->file.data = krealloc(file->file.data, new_size);
        file->file.size = new_size;
    }

    memcpy(file->file.data + fh->curr, buf, size);

    return size;
}
