#pragma once

// Everything file descriptor related

#include "types.h"
#include "schedule.h"
#include "../lib/unistd.h"

enum FD_TYPE {
    FD_UNKN,
    FD_VFS,
    FD_PIPE_I,
    FD_PIPE_O,
};

typedef struct fd_t {
    int n;
    enum FD_TYPE type;
    bool open;
    void * handle;
} fd_t;

fd_t * get_proc_fd(process_t * p, int fd);

int fd_close(process_t * p, int fd);
fd_t * add_fd(process_t * p);

size_t fd_read(process_t * p, int fd, void * buf, size_t count);
size_t fd_write(process_t * p, int fd, void * buf, size_t count);

int fd_stat(process_t * p, int fd, stat * out);

ll_head * copy_fds(process_t * p);
