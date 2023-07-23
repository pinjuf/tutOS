#pragma once

#include "types.h"

#define SIG_DFL 0
#define SIG_IGN 1

struct sigaction {
    int    sa_sig; // Signal number, set by syscall handler
    void (*sa_handler)(int);
    int    sa_flags;
} __attribute__((packed));

enum SIGNAL {
    SIGCHLD,
    SIGTERM,
    SIGKILL,
    SIGSTOP,
    SIGCONT,
    SIGINT,
    SIGILL,
    SIGFPE,
    SIGSEGV,
    SIGTRAP,
    SIGUSR1,
    SIGUSR2,
    SIGTSTP,
};

typedef struct stack_t {
    void * ss_sp;
    size_t ss_size;
    int ss_flags;
} __attribute__((packed)) stack_t;

// Flags for sigaction.sa_flags
#define SA_NOCLDWAIT 1
#define SA_ONSTACK   2

// Flags for stack_t.ss_flags
#define SS_DISABLE 1 // Don't use the stack
#define SS_ONSTACK 2 // Stack in use
