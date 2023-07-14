#include "schedule.h"
#include "util.h"
#include "mm.h"
#include "signal.h"

ll_head * processes;
process_t * current_process = NULL;
bool do_scheduling = false;
pid_t pid_counter = 1;

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
            memcpy(&current_process->regs, rf, sizeof(int_regframe_t));
        else
            current_process->to_exec = false;
        current_process->state = PROCESS_NOT_RUNNING;

        // Get the next or the current process
        do {
            current_process = ll_nextla(processes, current_process);
        } while (current_process->state == PROCESS_NONE || \
                 current_process->state == PROCESS_ZOMBIE);
    }

    memcpy(rf, &current_process->regs, sizeof(int_regframe_t));
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
