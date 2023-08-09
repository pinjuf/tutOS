#pragma once

#include "types.h"

typedef struct pipe_t {
    void * buf;
    size_t bufsize;
    volatile size_t head;
    volatile size_t tail;

    size_t head_fds;
    size_t tail_fds;
} pipe_t;

pipe_t * mkpipe(size_t bufsize);
void rmpipe(pipe_t * pipe);

size_t pipe_write(pipe_t * pipe, void * buf, size_t count);
size_t pipe_read(pipe_t * pipe, void * buf, size_t count);

void pipe_adj(pipe_t * pipe);
