#include "signal.h"

int sigemptyset(sigset_t * set) {
    set->sig_n = 0;

    return 0;
}

int sigfillset(sigset_t * set) {
    set->sig_n = sizeof(set->sigs)/sizeof(set->sigs[0]);

    for (uint32_t i = 0; i < set->sig_n; i++) {
        set->sigs[i] = i;
    }

    return 0;
}

int sigaddset(sigset_t * set, int signum) {
    if (sigismember(set, signum))
        return 0;

    if (set->sig_n == sizeof(set->sigs)/sizeof(set->sigs[0]))
        return -1;

    set->sigs[set->sig_n++] = signum;

    return 0;
}

int sigdelset(sigset_t * set, int signum) {
    if (!sigismember(set, signum))
        return 0;

    for (uint32_t i = 0; i < set->sig_n; i++) {
        if (set->sigs[i] == signum) {
            // Fill the hole with the last entry
            set->sigs[i] = set->sigs[--set->sig_n];
            break;
        }
    }

    return 0;
}
int sigismember(sigset_t * set, int signum) {
    bool out = false;

    for (uint32_t i = 0; i < set->sig_n; i++) {
        if (set->sigs[i] == signum) {
            out = true;
            break;
        }
    }

    return out;
}
