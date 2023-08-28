#include "pipe.h"

#include "mm.h"
#include "util.h"

pipe_t * mkpipe(size_t bufsize) {
    pipe_t * out = kmalloc(sizeof(pipe_t));

    out->buf = kmalloc(bufsize);
    out->bufsize = bufsize;
    out->head = 0;
    out->tail = 0;

    out->head_fds = 0; // No file descriptors refer to this pipe yet
    out->tail_fds = 0;

    return out;
}

void rmpipe(pipe_t * pipe) {
    // The structure itself is not free'd!
    if (pipe->buf) {
        kfree(pipe->buf);
        pipe->buf = NULL;
    }
}

size_t pipe_write(pipe_t * pipe, void * buf, size_t count) {
    size_t writable;
    do { // Block until sufficient space
        writable = pipe->bufsize - pipe->head;
    } while (writable < count);

    memcpy((void*)((uint64_t)pipe->buf + pipe->head), buf, count);
    pipe->head += count;
    return count;
}

size_t pipe_read(pipe_t * pipe, void * buf, size_t count) {
    size_t readable;
    do { // Block until sufficient data
        readable = pipe->head - pipe->tail;
    } while (readable < count);

    memcpy(buf, (void*)((uint64_t)pipe->buf + pipe->tail), count);
    pipe->tail += count;

    if (pipe->tail >= MIN(0x100, pipe->bufsize)) // Don't discard on every single tiny read
        pipe_adj(pipe);

    return count;
}

void pipe_adj(pipe_t * pipe) {
    // Adjusts a pipe by discarding already read data
    memcpy(pipe->buf, (void *)((uint64_t)pipe->buf + pipe->tail), pipe->head - pipe->tail);

    pipe->head -= pipe->tail;
    pipe->tail  = 0;
}

void pipe_resz(pipe_t * pipe, size_t newsize) {
    // Resize a pipe (preferably increasing the size)

    pipe->buf = krealloc(pipe->buf, newsize);
    pipe->bufsize = newsize;
}
