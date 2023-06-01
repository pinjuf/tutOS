#include "stdlib.h"
#include "syscall.h"

void * malloc(size_t n) {
    return (void*)syscall(336, n, 0, 0, 0, 0, 0);
}

void free(void * a) {
    syscall(337, (uint64_t)a, 0, 0, 0, 0, 0);
}
