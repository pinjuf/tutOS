#pragma once

#include "types.h"

#define SIG_DFL 0
#define SIG_IGN 1

struct sigaction {
    int    sa_sig; // Signal number, set by syscall handler
    void (*sa_handler)(int);
    int    sa_flags;
} __attribute__((packed));

// TutOS is not limited to the POSIX signals, any int_t is a valid signal
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
    SIGALRM,
    SIGPIPE,
};

typedef struct stack_t {
    void * ss_sp;
    size_t ss_size;
    int ss_flags;
} __attribute__((packed)) stack_t;

typedef struct sigset_t {
    uint32_t sig_n;
    int sigs[128]; // Unordered, every signal can only appear once
} __attribute((packed)) sigset_t;

// Flags for sigaction.sa_flags
#define SA_NOCLDWAIT 1
#define SA_ONSTACK   2

// Flags for stack_t.ss_flags
#define SS_DISABLE 1 // Don't use the stack
#define SS_ONSTACK 2 // Stack in use

enum SIG_MASKHOW {
    SIG_BLOCK,
    SIG_UNBLOCK,
    SIG_SETMASK,
};

int sigemptyset(sigset_t * set);
int sigfillset(sigset_t * set);
int sigaddset(sigset_t * set, int signum);
int sigdelset(sigset_t * set, int signum);
int sigismember(sigset_t * set, int signum);
