#include "vfs.h"
#include "mm.h"
#include "util.h"
#include "schedule.h"
#include "fd.h"

mountpoint_t * mountpoints;

void init_vfs() {
    mountpoints = (mountpoint_t *) kcalloc(sizeof(mountpoint_t) * MOUNTPOINTS_N);

    // Order matters by dependency
    mount(NULL, "/dev/", "devfs");
    mount("/dev/hdb1", "/", "ext2");
    mount(NULL, "/tmp/", "tmpfs");
}

int _mount(char * filepath, char * mountpoint, enum FILESYSTEM type) {
    // No FS type autodetection!
    if (type == FS_UNKN)
        return -1;

    char * filepath_clean = NULL;
    if (filepath) {
        filepath_clean = kmalloc(strlen(filepath)+1);
        memcpy(filepath_clean, filepath, strlen(filepath)+1);
    }

    if (filepath_clean && clean_path(filepath_clean) < 0) {
        kfree(filepath_clean);
        return -1;
    }

    char * mountpoint_clean = kmalloc(strlen(mountpoint)+2); // Null byte AND ending DIRSEP
    memcpy(mountpoint_clean, mountpoint, strlen(mountpoint)+1);
    mountpoint_clean[strlen(mountpoint)] = DIRSEP;
    mountpoint_clean[strlen(mountpoint)+1] = '\0';

    if (clean_path(mountpoint_clean) < 0) {
        kfree(filepath_clean);
        kfree(mountpoint_clean);
        return -1;
    }

    // Check if mountpoint is already mounted
    for (size_t i = 0; i < MOUNTPOINTS_N; i++) {
        if (!strcmp(mountpoints[i].path, mountpoint_clean)) {
            kwarn(__FILE__,__func__,"mountpoint already mounted");
            kfree(filepath_clean);
            kfree(mountpoint_clean);
            return -1;
        }
    }

    // Get a free mountpoint
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
    mnt->path     = mountpoint_clean;
    mnt->filepath = filepath_clean;

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

    if (etype == FS_UNKN)
        return -1;

    return _mount(filepath, mountpoint, etype);
}

int unmount(char * mountpoint) {
    // Clean the path
    char * mountpoint_clean = kmalloc(strlen(mountpoint)+2); // Null byte AND ending DIRSEP
    memcpy(mountpoint_clean, mountpoint, strlen(mountpoint)+1);
    mountpoint_clean[strlen(mountpoint)] = DIRSEP;
    mountpoint_clean[strlen(mountpoint)+1] = '\0';

    if (clean_path(mountpoint_clean) < 0) {
        kfree(mountpoint_clean);
        return -1;
    }

    // Find the mountpoint
    mountpoint_t * mnt = NULL;

    for (size_t i = 0; i < MOUNTPOINTS_N; i++) {
        if (!strcmp(mountpoints[i].path, mountpoint_clean)) {
            mnt = &mountpoints[i];
            break;
        }
    }

    if (!mnt) {
        kwarn(__FILE__,__func__,"mountpoint not found");
        kfree(mountpoint_clean);
        return -1;
    }

    kfree(mountpoint_clean);

    // Make sure no files are using this mountpoint
    for (size_t i = 0; i < ll_len(processes); i++) {
        process_t * proc = ll_get(processes, i);

        if (!IS_ALIVE(proc))
            continue;

        for (size_t j = 0; j < ll_len(proc->fds); j++) {
            fd_t * fd = ll_get(proc->fds, j);

            if (!fd->open)
                continue;

            if (fd->type != FD_VFS)
                continue;

            filehandle_t * fh = fd->handle;

            if (&mountpoints[fh->mountpoint] == mnt) {
                kwarn(__FILE__,__func__,"mountpoint busy");
                return -1; // Device busy
            }
        }    
    }

    // Call the filesystem's unmount function
    if (FILESYSTEMS[mnt->type].del_fs) {
        int status = FILESYSTEMS[mnt->type].del_fs(mnt);
        if (status < 0)
            return status;
    }

    // Close the file
    if (mnt->file) {
        kclose(mnt->file);
    }

    // Free the mountpoint
    kfree(mnt->path);
    kfree(mnt->filepath);
    memset(mnt, 0, sizeof(mountpoint_t));
    mnt->type = FS_UNKN;

    return 0;
}

int get_mountpoint(char * path) {
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

    return mountpoint;
}

filehandle_t * kopen(char * p, mode_t mode) {
    // Takes an ABSOLUTE path

    // We might do "unsanitary" stuff to the string
    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    if (clean_path(path) < 0) {
        kfree(path);
        return NULL;
    }

    // Get the correct mountpoint
    size_t mountpoint = get_mountpoint(path);

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

    filehandle_t * out = FILESYSTEMS[mountpoints[mountpoint].type].get_filehandle(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path), mode);

    if (out) {
        out->mountpoint = mountpoint;
        out->mode = mode;
        out->fd_refs = 0;
    }

    kfree(path);

    return out;
}

void kclose(filehandle_t * f) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].close_filehandle == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
        return;
    }

    FILESYSTEMS[mountpoints[f->mountpoint].type].close_filehandle(f);
}

size_t kread(filehandle_t * f, void * buf, size_t count) {
    if (FILESYSTEMS[mountpoints[f->mountpoint].type].read_file == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
        return 0;
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
        return 0;
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
        return NULL;
    }

    if (f->type != FILE_DIR) {
        kwarn(__FILE__,__func__,"file not a dir");
        return 0;
    }

    return FILESYSTEMS[mountpoints[f->mountpoint].type].read_dir(f);
}

int kcreate(char * path) {
    // Create a file
    // NOT LIKE creat(2), does not actually open the file

    char * path_clean = kmalloc(strlen(path)+1);
    memcpy(path_clean, path, strlen(path)+1);
    if (clean_path(path_clean) < 0) {
        kfree(path_clean);
        return -1;
    }

    path = path_clean;

    size_t mountpoint = get_mountpoint(path);
    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");
        return -1;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].create_file == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
        return -1;
    }

    int status = FILESYSTEMS[mountpoints[mountpoint].type].create_file(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path));

    kfree(path_clean);

    return status;
}

int kmkdir(char * path) {
    // Create a directory

    char * path_clean = kmalloc(strlen(path)+1);
    memcpy(path_clean, path, strlen(path)+1);
    if (clean_path(path_clean) < 0) {
        kfree(path_clean);
        return -1;
    }

    path = path_clean;

    size_t mountpoint = get_mountpoint(path);
    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");
        return -1;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].create_dir == NULL) {
        kwarn(__FILE__,__func__,"no driver support");
        return -1;
    }

    int status = FILESYSTEMS[mountpoints[mountpoint].type].create_dir(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path));

    kfree(path_clean);

    return status;
}

void fh_to_stat(filehandle_t * in, stat * out) {
    out->st_dev  = in->mountpoint;
    out->st_mode = in->type;
    out->st_size = in->size;
}

int remove_pathddots(char * path) {
    // Removes all ".."s from an absolute path

    const char sep[] = {DIRSEP, '.', '.', DIRSEP, '\0'}; // "/../"

    while (1) {
        char * next = strstr(path, (char*)sep);
        if (!next)
            break;

        // If the /../ is at the beginning, the string is invalid (TODO: Ignore perhaps? /../ => /, like POSIX does)
        if (next == path) {
            return -1;
        }

        // Find the previous DIRSEP
        char * prev = next - 1;
        while (*prev != DIRSEP) {
            prev--;
        }

        // Check that the jumped directory exists
        *next = '\0';
        if (kexists(path) != FILE_DIR) {
            return -1;
        }
        *next = DIRSEP;

        // Now we have something like this:
        //       v <-- v
        // /path/to/../file/
        //  prev^  ^next
        memmove(prev + 1, next + strlen((char*)sep), strlen(next + strlen((char*)sep)) + 1);
    }

    // Remove trailing "/.."

    char * curr = path + strlen(path) - 1;

    if (*curr == '.' && *(curr - 1) == '.' && *(curr - 2) == DIRSEP) {

        if (curr - 2 == path) {
            return -1;
        }

        // Check that the jumped directory exists
        curr[-2] = '\0';
        if (kexists(path) != FILE_DIR) {
            return -1;
        }
        curr[-2] = DIRSEP;

        char * prev = curr - 3;

        while (*prev != DIRSEP) {
            prev--;
        }

        *prev = '\0';
    }

    return 0;
}

int remove_pathdseps(char * path) {
    // Remove double directory separators in paths (//bin////test -> /bin/test)

    char * curr = path;

    while (*curr) {
        if (curr[0] == DIRSEP && curr[1] == DIRSEP) {
            memcpy(curr + 1, curr + 2, strlen(curr + 2) + 1);
        } else {
            curr++;
        }
    }

    return 0;
}

int remove_pathdots(char * path) {
    // Not to be confused with remove_pathddots, this removes all "."s from a path

    // Translate all "/./"s to "/"s
    strreplace(path, "/./", "/");

    // Remove trailing "/."
    char * curr = path + strlen(path) - 1;

    if (*curr == '.' && *(curr - 1) == DIRSEP) {
        *curr = '\0';
    }

    return 0;
}

int clean_path(char * path) {
    // Performs multiple "clean-ups" on an absolute path

    int status = remove_pathdseps(path);
    if (status < 0)
        return status;

    status = remove_pathddots(path);
    if (status < 0)
        return status;

    status = remove_pathdots(path);
    if (status < 0)
        return status;

    return 0;
}

int kunlink(char * p) {
    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    if (clean_path(path) < 0) {
        kfree(path);
        return -1;
    }

    // Get the correct mountpoint
    size_t mountpoint = get_mountpoint(path);

    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");

        kfree(path);

        return -1;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].unlink_file == NULL) {
        kwarn(__FILE__,__func__,"no driver support");

        kfree(path);

        return -1;
    }

    int status = FILESYSTEMS[mountpoints[mountpoint].type].unlink_file(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path));

    kfree(path);

    return status;
}

int krmdir(char * p) {
    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    if (clean_path(path) < 0) {
        kfree(path);
        return -1;
    }

    // Get the correct mountpoint
    size_t mountpoint = get_mountpoint(path);

    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");

        kfree(path);

        return -1;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].unlink_dir == NULL) {
        kwarn(__FILE__,__func__,"no driver support");

        kfree(path);

        return -1;
    }

    int status = FILESYSTEMS[mountpoints[mountpoint].type].unlink_dir(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path));

    kfree(path);

    return status;
}

int krmdir_recursive(char * p) {
    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    if (clean_path(path) < 0) {
        kfree(path);
        return -1;
    }

    // Get the correct mountpoint
    size_t mountpoint = get_mountpoint(path);

    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");

        kfree(path);

        return -1;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].unlink_dir_recursive == NULL) {
        kwarn(__FILE__,__func__,"no driver support");

        kfree(path);

        return -1;
    }

    int status = FILESYSTEMS[mountpoints[mountpoint].type].unlink_dir_recursive(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path));

    kfree(path);

    return status;
}
int kexists(char * p) {
    // Returns a negative number if the file does not exist,
    // or a (positive) FILETYPE

    char * path = kmalloc(strlen(p)+1);
    memcpy(path, p, strlen(p)+1);

    if (clean_path(path) < 0) {
        kfree(path);
        return -1;
    }

    // Get the correct mountpoint
    size_t mountpoint = get_mountpoint(path);

    if (mountpoint == (size_t)-1) {
        kwarn(__FILE__,__func__,"mountpoint not found");

        kfree(path);

        return -1;
    }

    if (FILESYSTEMS[mountpoints[mountpoint].type].exists == NULL) {
        kwarn(__FILE__,__func__,"no driver support");

        kfree(path);

        return -1;
    }

    int status = FILESYSTEMS[mountpoints[mountpoint].type].exists(&mountpoints[mountpoint], path + strmatchstart(mountpoints[mountpoint].path, path));

    kfree(path);

    return status;
}
