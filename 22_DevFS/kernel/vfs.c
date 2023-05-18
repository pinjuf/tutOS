#include "vfs.h"
#include "mm.h"
#include "util.h"

mountpoint_t * mountpoints;

void init_vfs() {
    mountpoints = (mountpoint_t *) kcalloc(sizeof(mountpoint_t) * MOUNTPOINTS_N);

    mountpoints[0].type = FS_EXT2;
    mountpoints[0].path = "/";
    mountpoints[0].p    = get_part(1, 0);

    mountpoints[1].type = FS_DEVFS;
    mountpoints[1].path = "/dev/";

    for (size_t i = 0; i < MOUNTPOINTS_N; i++) {
        if (mountpoints[i].type == FS_UNKN)
            continue;
        if (FILESYSTEMS[mountpoints[i].type].get_fs)
            mountpoints[i].internal_fs = FILESYSTEMS[mountpoints[i].type].get_fs(mountpoints[i].p);
    }
}

filehandle_t * kopen(char * p, enum FILEMODE mode) {

    // Get the string with the longest starting match
    // TODO: Shorten "x/y/../z" to "x/z"

    // We might do "unsanitary" stuff to the string
    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    if (strlen(path)>1 && path[strlen(path)-1] == DIRSEP) { // Remove ending "/" if the path is not "/" (root dir)
        path[strlen(path)-1] = 0;
    }

    size_t mountpoint = -1;
    size_t length = 0;
    for (size_t m = 0; m < MOUNTPOINTS_N; m++) {
        size_t match = strmatchstart(mountpoints[m].path, path);
        if (match > length && strlen(mountpoints[m].path) == match) {
            length = match;
            mountpoint = m;
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

    filehandle_t * out = FILESYSTEMS[mountpoints[mountpoint].type].get_filehandle(mountpoints[mountpoint].internal_fs, path + length, mode);

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

    if (f->mode != FILE_R) {
        kwarn(__FILE__,__func__,"file not open as r");
        return 0;
    }

    return FILESYSTEMS[mountpoints[f->mountpoint].type].read_file(f, buf, count);
}

size_t kwrite(filehandle_t * f, void * buf, size_t count) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].read_file == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
    }

    if (f->mode != FILE_W) {
        kwarn(__FILE__,__func__,"file not open as w");
        return 0;
    }

    return FILESYSTEMS[mountpoints[f->mountpoint].type].write_file(f, buf, count);
}
