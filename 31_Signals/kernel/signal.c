#include "signal.h"
#include "util.h"
#include "schedule.h"

void push_proc_sig(process_t * proc, int sig) {
    if (proc->sigqueue_sz == SIGQUEUE_SZ) {
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

    if (sa) {
        memcpy(sa, action, sizeof(struct sigaction));
    } else {
        struct sigaction * sa2 = ll_push(proc->sigactions);
        memcpy(sa2, sa, sizeof(struct sigaction));
    }
}
