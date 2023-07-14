#pragma once

#include "types.h"

#define SIG_DFL 0
#define SIG_IGN 1

struct sigaction {
    int    sa_sig; // Signal number, set by syscall handler
    void (*sa_handler)(int);
    int    sa_flags;
} __attribute__((packed));
