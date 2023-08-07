#include "fd.h"
#include "vfs.h"

fd_t * get_proc_fd(process_t * p, int fd) {
    fd_t * out = NULL;

    for (size_t i = 0; i < ll_len(p->fds); i++) {
        if (((fd_t *)ll_get(p->fds, i))->n == fd) {
            out = (fd_t *)ll_get(p->fds, i);
            break;
        }
    }

    return out;
}

int fd_close(process_t * p, int fd) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return -1;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            kclose(fh);

            return 0;
        }
        default:
            return -1;
    }
}
