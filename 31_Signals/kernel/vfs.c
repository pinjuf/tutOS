#include "vfs.h"
#include "mm.h"
#include "util.h"

mountpoint_t * mountpoints;

void init_vfs() {
    mountpoints = (mountpoint_t *) kcalloc(sizeof(mountpoint_t) * MOUNTPOINTS_N);

    mount(NULL, "/dev/", "devfs");
    mount("/dev/hdb1", "/", "ext2");
}

int _mount(char * filepath, char * mountpoint, enum FILESYSTEM type) {
    // Returns the mountpoint number if successful, else a negative error

    mountpoint_t * mnt = NULL;
    for (size_t i = 0; i < MOUNTPOINTS_N; i++) {
        if (mountpoints[i].type == FS_UNKN) {
            mnt = &mountpoints[i];
            break;
        }
    }

    if (!mountpoint)
        return -1;

    memset(mnt, 0, sizeof(mountpoint_t));

    mnt->type     = type;
    mnt->path     = mountpoint;
    mnt->filepath = filepath;

    if (mnt->filepath) {
        mnt->file = kopen(mnt->filepath, FILESYSTEMS[type].default_rw);
        if (!mnt->file)
            return -1;
    }

    if (FILESYSTEMS[type].get_fs) {
        mnt->internal_fs = FILESYSTEMS[type].get_fs(mnt->file);
    }

    return 0;
}

int mount(char * filepath, char * mountpoint, char * type) {
    enum FILESYSTEM etype = FS_UNKN;

    for (size_t i = 0; i < sizeof(FILESYSTEMS)/sizeof(FILESYSTEMS[0]); i++) {
        if (!strcmp(type, (char*)FILESYSTEMS[i].name)) {
            etype = i;
        }
    }

    return _mount(filepath, mountpoint, etype);
}

filehandle_t * kopen(char * p, mode_t mode) {

    // Get the string with the longest starting match
    // TODO: Shorten "x/y/../z" to "x/z"

    // We might do "unsanitary" stuff to the string
    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    size_t mountpoint = -1;
    size_t length = 0;
    for (size_t m = 0; m < MOUNTPOINTS_N; m++) {
        // 2 scenarios:
        //
        // 1) path equals JUST the mountpoint (without trailing DIRSEP), e.g. "/dev"
        // 2) path equals mountpoint AND a DIRSEP (and possibly additional path tokens), e.g. "/dev/", "/dev/tty0"
        size_t match = strmatchstart(mountpoints[m].path, path);

        if (match == strlen(mountpoints[m].path) && match > length) { // Scenario 2
            mountpoint = m;
            length = match;
        } else if (match == strlen(mountpoints[m].path) - 1 && strlen(path) == match) { // Scenario 1
            mountpoint = m;
            length = match;
            break;
        }
    }

    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");

        kfree(path);

        return NULL;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].get_filehandle == NULL) {
        kwarn(__FILE__,__func__,"no driver support");

        kfree(path);

        return NULL;
    }

    filehandle_t * out = FILESYSTEMS[mountpoints[mountpoint].type].get_filehandle(&mountpoints[mountpoint], path + length, mode);

    if (out) {
        out->mountpoint = mountpoint;
        out->mode = mode;
    }

    kfree(path);

    return out;
}

void kclose(filehandle_t * f) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].close_filehandle == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
    }

    FILESYSTEMS[mountpoints[f->mountpoint].type].close_filehandle(f);
}

size_t kread(filehandle_t * f, void * buf, size_t count) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].read_file == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
    }

    if (!(f->mode & O_RDONLY)) {
        kwarn(__FILE__,__func__,"file not open as r");
        return 0;
    }

    return FILESYSTEMS[mountpoints[f->mountpoint].type].read_file(f, buf, count);
}

size_t kreadat(filehandle_t * f, size_t off, void * buf, size_t count) {
    // reads from an offset and preserves the cursor

    size_t orig = f->curr;
    f->curr = off;
    size_t read = kread(f, buf, count);
    f->curr = orig;
    return read;
}

size_t kwrite(filehandle_t * f, void * buf, size_t count) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].read_file == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
    }

    if (!(f->mode & O_WRONLY)) {
        kwarn(__FILE__,__func__,"file not open as w");
        return 0;
    }

    return FILESYSTEMS[mountpoints[f->mountpoint].type].write_file(f, buf, count);
}

size_t kwriteat(filehandle_t * f, size_t off, void * buf, size_t count) {
    size_t orig = f->curr;
    f->curr = off;
    size_t written = kwrite(f, buf, count);
    f->curr = orig;
    return written;
}

dirent * kreaddir(filehandle_t * f) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].read_dir == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
    }

    if (f->type != FILE_DIR) {
        kwarn(__FILE__,__func__,"file not a dir");
        return 0;
    }

    return FILESYSTEMS[mountpoints[f->mountpoint].type].read_dir(f);
}

void fh_to_stat(filehandle_t * in, stat * out) {
    out->st_dev  = in->mountpoint;
    out->st_mode = in->type;
    out->st_size = in->size;
}
