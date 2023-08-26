#include "fd.h"
#include "vfs.h"
#include "util.h"
#include "pipe.h"
#include "signal.h"
#include "mm.h"

fd_t * get_proc_fd(process_t * p, int fd) {
    fd_t * out = NULL;

    for (size_t i = 0; i < ll_len(p->fds); i++) {
        fd_t * fd_struct = (fd_t *) ll_get(p->fds, i);
        if (fd_struct->n == fd && fd_struct->open) {
            out = (fd_t *)ll_get(p->fds, i);
            break;
        }
    }

    return out;
}

fd_t * add_fd(process_t * p) {
    fd_t * out = ll_push(p->fds);
    out->n = p->fd_n++;
    out->open = true;
    return out;
}

int fd_close(process_t * p, int fd) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    // Invalid fd? Already closed?
    if (fd_struct == NULL) {
        return -1;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            fh->fd_refs--;    // No other file descriptors are connected to this
            if (!fh->fd_refs) {
                kclose(fh);
            }

            // Closed FDs stay in the list!
            fd_struct->open = false;

            return 0;
        }
        case FD_PIPE_I: {
            pipe_t * pipe = fd_struct->handle;

            pipe->head_fds--;
            if (!pipe->tail_fds && !pipe->head_fds) {
                rmpipe(pipe);
                kfree(pipe);
            }

            fd_struct->open = false;

            return 0;
        }
        case FD_PIPE_O: {
            pipe_t * pipe = fd_struct->handle;

            pipe->tail_fds--;
            if (!pipe->tail_fds) {
                rmpipe(pipe);

                // Pipe completely abandoned?
                if (!pipe->head_fds) {
                    kfree(pipe);
                }
            }

            fd_struct->open = false;

            return 0;
        }
        default:
            return -1;
    }
}

size_t fd_read(process_t * p, int fd, void * buf, size_t count) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return 0;
    }

    if (!fd_struct->open) {
        return 0;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            return kread(fh, buf, count);
        }
        case FD_PIPE_O: {
            pipe_t * pipe = fd_struct->handle;

            // No one to write and pipe empty? (EOF)
            if (!pipe->head_fds && (pipe->head == pipe->tail)) {
                return 0;
            }

            return pipe_read(pipe, buf, count);
        }
        default:
            return 0;
    }
}

size_t fd_write(process_t * p, int fd, void * buf, size_t count) {
    fd_t * fd_struct = get_proc_fd(p, fd);

    if (fd_struct == NULL) {
        return 0;
    }

    if (!fd_struct->open) {
        return 0;
    }

    switch (fd_struct->type) {
        case FD_VFS: {
            filehandle_t * fh = fd_struct->handle;

            if (fh->mode & O_APPEND) {
                fh->curr = fh->size;
            }

            return kwrite(fh, buf, count);
        }
        case FD_PIPE_I: {
            pipe_t * pipe = fd_struct->handle;

            // No one to read? (also means the pipe buffer has been free'd)
            if (!pipe->tail_fds) {
                push_proc_sig(p, SIGPIPE);
                return 0;
            }

            return pipe_write(pipe, buf, count);
        }
        default:
            return 0;
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
        case FD_PIPE_I:
        case FD_PIPE_O: {
            out->st_dev = SIZE_MAX; // Pipes are anonymous
            out->st_mode = FILE_PIPE;
            out->st_size = ((pipe_t*)fd_struct->handle)->bufsize;

            return 0;
        }
        default:
            return -1;
    }
}

ll_head * copy_fds(process_t * p) {
    ll_head * out = ll_copy(p->fds);

    for (size_t i = 0; i < ll_len(out); i++) {
        fd_t * fd = (fd_t *) ll_get(out, i);

        switch (fd->type) {
            case FD_VFS: {
                filehandle_t * fh = fd->handle;

                fh->fd_refs++;
                break;
            }

            case FD_PIPE_I: {
                pipe_t * pipe = fd->handle;

                pipe->head_fds++;
                break;
            }

            case FD_PIPE_O: {
                pipe_t * pipe = fd->handle;

                pipe->tail_fds++;
                break;
            }

            default:
                return NULL;
        }
    }

    return out;
}
