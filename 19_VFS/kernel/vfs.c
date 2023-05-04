#include "vfs.h"
#include "mm.h"
#include "util.h"

mountpoint_t * mountpoints;

void init_vfs() {
    mountpoints = (mountpoint_t *) kcalloc(sizeof(mountpoint_t) * MOUNTPOINTS_N);

    mountpoints[0].type = FS_EXT2;
    mountpoints[0].path = "/";
    mountpoints[0].p    = get_part(1, 0);

    for (size_t i = 0; i < MOUNTPOINTS_N; i++) {
        if (mountpoints[i].type == FS_UNKN)
            continue;
        mountpoints[i].internal_fs = FILESYSTEMS[mountpoints[i].type].get_fs(mountpoints[i].p);
    }
}

filehandle_t * kopen(char * path) {

    // Get the string with the longest starting match
    // TODO: Shorten "x/y/../z" to "x/z"

    if (path[strlen(path)-1] == DIRSEP) { // Remove ending "/"
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
        return NULL;
    }

    return FILESYSTEMS[mountpoints[mountpoint].type].get_filehandle(mountpoints[mountpoint].internal_fs, path + length);
}
