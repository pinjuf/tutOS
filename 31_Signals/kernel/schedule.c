#include "schedule.h"
#include "util.h"
#include "mm.h"
#include "main.h"
#include "signal.h"

ll_head * processes;
process_t * current_process = NULL;
bool do_scheduling = false;
pid_t pid_counter = INIT_PID;

void init_scheduling() {
    processes = create_ll(sizeof(process_t));
}

void schedule(void * regframe_ptr) {
    int_regframe_t * rf = (int_regframe_t*)regframe_ptr;

    bool process_to_run = false;
    for (size_t i = 0; i < ll_len(processes); i++) {
        if (((process_t*)ll_get(processes, i))->state != PROCESS_NONE && \
            ((process_t*)ll_get(processes, i))->state != PROCESS_ZOMBIE) {
            process_to_run = true;
        }
    }

    if (!process_to_run) {
        kwarn(__FILE__,__func__,"no process to run, halting");
        asm volatile ("hlt");
    }

    if (!current_process) { // First time loading in? / Last process killed?
        // Look for the first process we can run

        for (size_t i = 0; i < ll_len(processes); i++) {
            if (((process_t*)ll_get(processes, i))->state != PROCESS_NONE && \
                ((process_t*)ll_get(processes, i))->state != PROCESS_ZOMBIE) {
                current_process = ll_get(processes, i);
            }
        }
    } else {
        // Save current process, given that it is not about to undergo a context change
        if (!current_process->to_exec)
            read_proc_regs(current_process, rf);
        else
            current_process->to_exec = false;
        current_process->state = PROCESS_NOT_RUNNING;

        // Get the next or the current process
        do {
            current_process = ll_nextla(processes, current_process);
        } while (current_process->state == PROCESS_NONE || \
                 current_process->state == PROCESS_ZOMBIE);
    }

    write_proc_regs(current_process, rf);
    current_process->state = PROCESS_RUNNING;

    for (size_t i = 0; i < current_process->pagemaps_n; i++) {
        mmap_pages(current_process->pagemaps[i].virt,
                   current_process->pagemaps[i].phys,
                   current_process->pagemaps[i].attr,
                   current_process->pagemaps[i].n);
    }

    if (current_process->to_fork) {
        current_process->to_fork = false;

        process_t * child = add_process();

        current_process->latest_child = child->pid;

        memcpy(child, current_process, sizeof(process_t));

        child->pid = current_process->latest_child;

        child->state = PROCESS_NOT_RUNNING;
        child->latest_child = 0;
        child->parent = current_process->pid;

        child->sigactions = ll_copy(current_process->sigactions);

        child->pagemaps = kmalloc(child->pagemaps_n * sizeof(pagemap_t));
        for (size_t i = 0; i < child->pagemaps_n; i++) {
            child->pagemaps[i].virt = current_process->pagemaps[i].virt;
            child->pagemaps[i].n    = current_process->pagemaps[i].n;
            child->pagemaps[i].attr = current_process->pagemaps[i].attr;

            void * buf = alloc_pages(current_process->pagemaps[i].n);

            child->pagemaps[i].phys = virt_to_phys(buf);

            memcpy(buf, current_process->pagemaps[i].virt, current_process->pagemaps[i].n * PAGE_SIZE);
        }
    }

    if (current_process->to_sigreturn) {
        current_process->sighandling  = false;
        current_process->to_sigreturn = false;
        write_proc_regs(current_process, rf);
    }

    if (!current_process->sighandling && current_process->sigqueue_sz) {
        // There is a pending signal a process is ready to handle

        int signum            = pop_proc_sig(current_process);
        struct sigaction * sa = get_proc_sigaction(current_process, signum);

        // Unhandleable signals
        switch (signum) {
            case SIGKILL:
                kputs(" < PROCESS #");
                kputdec(current_process->pid);
                kputs(" KILLED (SIGKILL) > ");

                kill_process(current_process, UINT8_MAX);
                current_process = NULL;

                // Just run the scheduler over everything again
                schedule(rf);
                return;

            default:
                break;
        }

        if (!sa || ((uint64_t)sa->sa_handler == SIG_DFL)) {
            // Default handler
            switch (signum) {
                case SIGTERM:
                    kputs(" < PROCESS #");
                    kputdec(current_process->pid);
                    kputs(" KILLED (SIGTERM) > ");

                    kill_process(current_process, UINT8_MAX);
                    current_process = NULL;

                    // Just run the scheduler over everything again
                    schedule(rf);
                    return;

                default:
                    break;

            }

        } else if ((uint64_t)sa->sa_handler != SIG_IGN) {
            // A handler has been registered by the program and must now be jumped to.

            current_process->sighandling  = true;
            current_process->to_sigreturn = false;

            // Just copy pretty much the entire context
            memcpy(&current_process->sigregs, &current_process->regs, sizeof(int_regframe_t));

            // If a program is running in usermode, all of its handlers are too!
            current_process->sigregs.cs = current_process->kmode ? (1*8) : ((6*8) | 3);
            current_process->sigregs.ss = current_process->kmode ? (2*8) : ((5*8) | 3);

            // TODO: sigaltstack

            current_process->sigregs.rip = (uint64_t)sa->sa_handler;
            current_process->sigregs.rdi = signum;

            write_proc_regs(current_process, rf);

        } // SIG_IGN is, well, ignored
    }
}

void proc_set_args(process_t * proc, int argc, char * argv[]) {
    proc->regs.rdi = proc->argc = argc;
    proc->regs.rsi = (uint64_t) (proc->argv = argv);
}

process_t * add_process() {
    process_t * out = ll_push(processes);
    memset(out, 0, sizeof(process_t));
    out->pid = pid_counter++;
    out->sigactions = create_ll(sizeof(struct sigaction));
    return out;
}

process_t * get_proc_by_pid(pid_t pid) {
    for (size_t i = 0; i < ll_len(processes); i++) {
        process_t * p = ll_get(processes, i);

        if (p->pid == pid)
            return p;
    }

    return NULL;
}

void clear_none_procs() {
    // Removes processes we no longer need

    for (size_t i = 0; i < ll_len(processes); i++) {
        process_t * p = ll_get(processes, i);

        if (p == NULL)
            break;

        if (p->state == PROCESS_NONE) {
            ll_del(processes, i);
            i--;
        }
    }
}

void write_proc_regs(process_t * proc, int_regframe_t * regs) {
    if (proc->sighandling)
        memcpy(regs, &proc->sigregs, sizeof(int_regframe_t));
    else
        memcpy(regs, &proc->regs, sizeof(int_regframe_t));
}

void read_proc_regs(process_t * proc, int_regframe_t * regs) {
    if (proc->sighandling)
        memcpy(&proc->sigregs, regs, sizeof(int_regframe_t));
    else
        memcpy(&proc->regs, regs, sizeof(int_regframe_t));
}

void free_pagemaps(pagemap_t * maps, size_t n) {
    for (size_t i = 0; i < n; i++) {
        uint64_t addr = (uint64_t)maps[i].phys;
        // Dirty, I know...
        addr -= HEAP_PHYS;
        addr += HEAP_VIRT;
        free_pages((void*)addr, maps[i].n);
    }
}

void kill_process(process_t * proc, uint8_t return_code) {
    // Free the memory
    free_pagemaps(proc->pagemaps, proc->pagemaps_n);
    kfree(proc->pagemaps);
    destroy_ll(proc->sigactions);

    proc->state = PROCESS_ZOMBIE;
    proc->exitcode = return_code;

    // Notify the parent that the child died
    if (proc->parent && get_proc_by_pid(proc->parent)) {
        process_t * parent = get_proc_by_pid(proc->parent);

        push_proc_sig(parent, SIGCHLD);

        // Check for SA_NOCLDWAIT (no zombie)
        struct sigaction * sa_sigchld = get_proc_sigaction(parent, SIGCHLD);
        if (sa_sigchld->sa_flags & SA_NOCLDWAIT)
            proc->state = PROCESS_NONE;
    }
}
