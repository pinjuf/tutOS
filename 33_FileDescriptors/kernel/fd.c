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

fd_t * add_fd(process_t * p) {
    fd_t * out = ll_push(p->fds);
    out->n = p->fd_n++;
    return out;
}

int fd_close(process_t * p, int fd) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return -1;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            // TODO: This would close it globally!

            //filehandle_t * fh = fd_struct->handle;

            //kclose(fh);

            ll_delp(p->fds, fd_struct);

            return 0;
        }
        default:
            return -1;
    }
}

size_t fd_read(process_t * p, int fd, void * buf, size_t count) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return -1;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            return kread(fh, buf, count);
        }
        default:
            return -1;
    }
}

size_t fd_write(process_t * p, int fd, void * buf, size_t count) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return -1;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            return kwrite(fh, buf, count);
        }
        default:
            return -1;
    }
}

int fd_stat(process_t * p, int fd, stat * out) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return -1;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            fh_to_stat(fh, out);

            return 0;
        }
        default:
            return -1;
    }
}
