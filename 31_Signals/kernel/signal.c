#include "signal.h"
#include "util.h"
#include "schedule.h"

void push_proc_sig(process_t * proc, int sig) {
    if (proc->sigqueue_sz >= SIGQUEUE_SZ) {
        kwarn(__FILE__,__func__,"signal queue full");
        return;
    }

    proc->sigqueue[proc->sigqueue_sz++] = sig;
}

int pop_proc_sig(process_t * proc) {
    if (!proc->sigqueue_sz) {
        kwarn(__FILE__,__func__,"signal queue empty");
        return -1;
    }

    int out = proc->sigqueue[0];
    memcpy(&proc->sigqueue[0], &proc->sigqueue[1], sizeof(int) * (proc->sigqueue_sz-- - 1));
    return out;
}

struct sigaction * get_proc_sigaction(process_t * proc, int sig) {
    // The list may already have been kfree'd
    if (proc->state == PROCESS_ZOMBIE || \
        proc->state == PROCESS_NONE)
        return NULL;

    struct sigaction * out = NULL;

    for (size_t i = 0; i < ll_len(proc->sigactions); i++) {
        struct sigaction * sa = ll_get(proc->sigactions, i);

        if (sa->sa_sig == sig) {
            out = sa;
            break;
        }
    }

    return out;
}

void register_sigaction(process_t * proc, struct sigaction * action) {
    struct sigaction * sa = get_proc_sigaction(proc, action->sa_sig);

    if (!sa) {
        sa = ll_push(proc->sigactions);
    }

    memcpy(sa, action, sizeof(struct sigaction));
}

void del_proc_sig(process_t * proc, size_t index) {
    if (index >= proc->sigqueue_sz) {
        kwarn(__FILE__,__func__,"index too high");
        return;
    }

    memcpy(&proc->sigqueue[index], &proc->sigqueue[index + 1], (--proc->sigqueue_sz - index) * sizeof(int));
}
