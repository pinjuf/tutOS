#include "schedule.h"
#include "util.h"
#include "mm.h"

process_t * processes;
process_t * current_process = NULL;
bool do_scheduling = false;

void init_scheduling() {
    processes = kcalloc(sizeof(process_t) * MAX_PROCESSES);
}

void schedule(void * regframe_ptr) {
    int_regframe_t * rf = (int_regframe_t*)regframe_ptr;

    bool process_to_run = false;
    for (size_t i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state != PROCESS_NONE)
            process_to_run = true;
    }

    if (!process_to_run) {
        kwarn(__FILE__,__func__,"no process to run");
        return;
    }

    if (!current_process) { // First time loading in? / Last process killed?
        // Look for the first process we can run
        for (size_t i = 0; i < MAX_PROCESSES; i++) {
            if (processes[i].state == PROCESS_NOT_RUNNING) {
                current_process = &processes[i];
                break;
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
        pid_t current_pid = PROC_PTR_TO_PID(current_process);

        for (size_t i = 1; i <= MAX_PROCESSES; i++) {
            if (processes[(current_pid+i)%MAX_PROCESSES].state == PROCESS_RUNNING \
             || processes[(current_pid+i)%MAX_PROCESSES].state == PROCESS_NOT_RUNNING) {
                current_process = &processes[(current_pid+i)%MAX_PROCESSES];
                break;
            }
        }
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

        process_t * child = NULL;
        for (size_t i = 0; i < MAX_PROCESSES; i++) {
            if (processes[i].state == PROCESS_NONE) {
                child = &processes[i];
                break;
            }
        }

        if (!child) {
            kwarn(__FILE__,__func__,"no child process available");
            return;
        }

        current_process->latest_child = PROC_PTR_TO_PID(child);

        memcpy(child, current_process, sizeof(process_t));

        child->state = PROCESS_NOT_RUNNING;
        child->latest_child = 0;
        child->parent = PROC_PTR_TO_PID(current_process);

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
