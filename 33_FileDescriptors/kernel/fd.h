#pragma once

// Everything file descriptor related

#include "types.h"
#include "schedule.h"

enum FD_TYPE {
    FD_UNKN,
    FD_VFS,
    // TODO: Add pipes
};

typedef struct fd_t {
    int n;
    enum FD_TYPE type;
    void * handle;
} fd_t;

fd_t * get_proc_fd(process_t * p, int fd);

int fd_close(process_t * p, int fd);
