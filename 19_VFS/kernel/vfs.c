#include "vfs.h"
#include "mm.h"

mountpoint_t * mountpoints;

void init_vfs() {
    mountpoints = (mountpoint_t *) kcalloc(sizeof(mountpoint_t) * MOUNTPOINTS_N);

    mountpoints[0].type = FS_EXT2;
    mountpoints[0].path = "/";
    mountpoints[0].p    = get_part(1, 0);

    for (size_t i = 0; i < MOUNTPOINTS_N; i++) {
        mountpoints[i].internal_fs = FILESYSTEMS[mountpoints[i].type].get_fs(mountpoints[i].p);
    }
}
