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

    // Create the . file (no .. in root directory)
    tmpfs_file_t * dot = ll_push(root->dir.files);
    dot->type = FILE_DIR;
    strcpy(dot->name, ".");
    dot->dir.files = root->dir.files;

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

    tmpfs_file_t * curr = tmpfs->root;

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

        tmpfs_file_t * next = tmpfs_getfiledir(curr->dir.files, token);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return NULL;
        }

        if (next->type != FILE_DIR && *p != 0) {
            kwarn(__FILE__,__func__,"not a directory");
            return NULL;
        }

        curr = next;
    }

    filehandle_t * fh = kmalloc(sizeof(filehandle_t));
    tmpfs_filehandle_t * intern = fh->internal_file = kmalloc(sizeof(tmpfs_filehandle_t));

    if (mode & O_TRUNC) {
        if (curr->type != FILE_REG) {
            kwarn(__FILE__,__func__,"cannot truncate a non-file");
            return NULL;
        }

        curr->file.size = 0;
        curr->file.data = krealloc(curr->file.data, 0);
    }

    intern->file = curr;
    fh->curr = 0;
    fh->type = curr->type;

    switch (fh->type) {
        case FILE_REG:
            fh->size = curr->file.size;
            break;
        case FILE_DIR:
            fh->size = ll_len(curr->dir.files);
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
    // Get the filename, and separate it from the path by nulling the last DIRSEP
    char * o = path + strlen(path) - 1;
    while (*o != DIRSEP && o > path)
        o--;
    memcpy(filename, o, strlen(o) + 1);
    *o = 0;
    if (filename[0] == DIRSEP)
        strcpy(filename, filename + 1);

    char token[256];

    // Get the dir we're creating the file in
    tmpfs_file_t * dir = tmpfs->root;

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

        tmpfs_file_t * next = tmpfs_getfiledir(dir->dir.files, token);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return -1;
        }

        if (next->type != FILE_DIR) {
            kwarn(__FILE__,__func__,"not a directory");
            return -1;
        }

        dir = next;
    }

    if (tmpfs_getfiledir(dir->dir.files, filename)) {
        kwarn(__FILE__,__func__,"file already exists");
        return -1;
    }

    tmpfs_file_t * file = ll_push(dir->dir.files);

    file->type = FILE_REG;
    strcpy(file->name, filename);
    file->file.size = 0;
    file->file.data = NULL;

    return 0;
}

int tmpfs_createdir(void * m, char * path) {
    mountpoint_t * mnt = m;
    tmpfs_t * tmpfs = mnt->internal_fs;

    char filename[256];
    char * o = path + strlen(path) - 1;
    while (*o != DIRSEP && o > path)
        o--;
    memcpy(filename, o, strlen(o) + 1);
    *o = 0;
    if (filename[0] == DIRSEP)
        strcpy(filename, filename + 1);

    char token[256];

    // Get the dir we're creating the file in
    tmpfs_file_t * dir = tmpfs->root;

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

        tmpfs_file_t * next = tmpfs_getfiledir(dir->dir.files, token);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return -1;
        }

        if (next->type != FILE_DIR) {
            kwarn(__FILE__,__func__,"not a directory");
            return -1;
        }

        dir = next;
    }

    if (tmpfs_getfiledir(dir->dir.files, filename)) {
        kwarn(__FILE__,__func__,"directory already exists");
        return -1;
    }

    tmpfs_file_t * file = ll_push(dir->dir.files);

    file->type = FILE_DIR;
    strcpy(file->name, filename);
    file->dir.files = create_ll(sizeof(tmpfs_file_t));

    // Create the . and .. files
    tmpfs_file_t * dot = ll_push(file->dir.files);
    tmpfs_file_t * dotdot = ll_push(file->dir.files);
    dot->type = dotdot->type = FILE_DIR;
    strcpy(dot->name, ".");
    strcpy(dotdot->name, "..");
    dot->dir.files = file->dir.files;
    dotdot->dir.files = dir->dir.files;

    return 0;
}

size_t tmpfs_readfile(void * f, void * buf, size_t size) {
    filehandle_t * fh = f;
    tmpfs_filehandle_t * tmpfs_fh = fh->internal_file;
    tmpfs_file_t * file = tmpfs_fh->file;

    size_t to_read = size;
    if (to_read > file->file.size - fh->curr)
        to_read = file->file.size - fh->curr;

    memcpy(buf, (void*)((size_t)file->file.data + fh->curr), to_read);

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

    memcpy((void*)((size_t)file->file.data + fh->curr), buf, size);

    return size;
}

int tmpfs_unlinkfile(void * m, char * path) {
    char token[256];

    mountpoint_t * mnt = (mountpoint_t *) m;
    tmpfs_t * tmpfs = (tmpfs_t *) mnt->internal_fs;

    tmpfs_file_t * prevdir = NULL;
    tmpfs_file_t * curr = tmpfs->root;

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

        tmpfs_file_t * next = tmpfs_getfiledir(curr->dir.files, token);

        if (!next) {
            kwarn(__FILE__,__func__,"file not found");
            return -1;
        }

        if (next->type != FILE_DIR && *p != 0) {
            kwarn(__FILE__,__func__,"not a directory");
            return -1;
        }

        prevdir = curr;
        curr = next;
    }

    if (curr->type != FILE_REG) {
        kwarn(__FILE__,__func__,"cannot unlink a non-file");
        return -1;
    }

    if (prevdir == NULL) {
        kwarn(__FILE__,__func__,"cannot unlink root directory");
        return -1;
    }

    kfree(curr->file.data);
    ll_delp(prevdir->dir.files, curr);

    return 0;
}
