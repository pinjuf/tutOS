#pragma once

#include "types.h"
#include "schedule.h"

#define SIG_DFL 0
#define SIG_IGN 1

// Some deviation from the POSIX standard, I know...
struct sigaction {
    int    sa_sig; // Signal number, set by syscall handler
    void (*sa_handler)(int);
    int    sa_flags; // TODO: Add flags (e.g. SA_NOCLDWAIT)
} __attribute__((packed));

enum SIGNAL { // TODO: Fill this up
    SIGCHLD,
    SIGTERM,
    SIGKILL,
};

void push_proc_sig(process_t * proc, int sig);
int pop_proc_sig(process_t * proc);
struct sigaction * get_proc_sigaction(process_t * proc, int sig);
void register_sigaction(process_t * proc, struct sigaction * action);
