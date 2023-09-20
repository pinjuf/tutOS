#include "schedule.h"
#include "util.h"
#include "mm.h"
#include "main.h"
#include "signal.h"
#include "isr.h"
#include "fd.h"
#include "vfs.h"
#include "apic.h"
#include "ap.h"

ll_head * processes;
bool do_scheduling = false;
pid_t pid_counter = INIT_PID;
volatile uint64_t schedule_ticks = 0;
spinlock_t procchoice_lock = 0; // Lock during process choice

void init_scheduling() {
    processes = create_ll(sizeof(process_t));
}

void schedule(void * regframe_ptr) {
    int_regframe_t * rf = (int_regframe_t*)regframe_ptr;

    cpu_coreinfo_t * core = get_core();

    if (!core->current_process) { // First time loading in? / Last process killed?
        if (!ll_len(processes)) {
            kwarn(__FILE__,__func__,"process list empty (halting)");
            while (1);
        }

        core->current_process = ll_get(processes, 0);
    } else {
        // Save current process, given that it is not about to undergo a context change
        if (!core->current_process->to_exec)
            read_proc_regs(core->current_process, rf);
        else
            core->current_process->to_exec = false;

        core->current_process->core = PROCESS_CORE_NONE;
    }

    // Get the next or the current process
    // This will loop indefinetly if there is nothing to run!
    spinlock_acquire(&procchoice_lock);

    do {
        core->current_process = ll_nextla(processes, core->current_process);

        if (!IS_ALIVE(core->current_process))
            continue;

        // Note: If the signal queue is full, no SIGCONT will be able to be delivered!
        size_t sigstop_prio   = proc_has_sig(core->current_process, SIGSTOP);
        size_t sigtstp_prio   = proc_has_sig(core->current_process, SIGTSTP);
        struct sigaction * sa = get_proc_sigaction(core->current_process, SIGTSTP);
        if (!sa || ((uint64_t)sa->sa_handler == SIG_DFL)) {
            sigstop_prio = MAX(sigstop_prio, sigtstp_prio);
        }

        if ((core->current_process->state == PROCESS_RUNNING) && sigstop_prio) {
            core->current_process->state = PROCESS_STOPPED;
        }

        // Is there a more RECENT SIGCONT in the queue?
        size_t sigcont_prio = proc_has_sig(core->current_process, SIGCONT);
        if ((core->current_process->state == PROCESS_STOPPED) && (sigcont_prio > sigstop_prio)) {
            core->current_process->state = PROCESS_RUNNING;
        }
    } while ((core->current_process->state != PROCESS_RUNNING) \
          || (core->current_process->core  != PROCESS_CORE_NONE)); // Only execute running processes that are not already running on a core

    core->current_process->core = core->apic_id; // Make sure no other core takes this process

    core->current_process->regs.cr3 = (uint64_t)virt_to_phys(core->pml4t); // Different core, different page tables

    spinlock_release(&procchoice_lock);

    write_proc_regs(core->current_process, rf);

    // Map the processes memory
    for (size_t i = 0; i < core->current_process->pagemaps_n; i++) {
        mmap_pages(core->current_process->pagemaps[i].virt,
                   core->current_process->pagemaps[i].phys,
                   core->current_process->pagemaps[i].attr,
                   core->current_process->pagemaps[i].n);
    }

    if (core->current_process->to_fork) {
        core->current_process->to_fork = false;

        process_t * child = add_process();

        core->current_process->latest_child = child->pid;

        memcpy(child, core->current_process, sizeof(process_t));

        child->pid = core->current_process->latest_child;
        child->latest_child = 0;
        child->parent = core->current_process->pid;
        child->core = PROCESS_CORE_NONE;

        child->sigactions = ll_copy(core->current_process->sigactions);

        child->fds        = copy_fds(core->current_process);

        child->pagemaps = kmalloc(child->pagemaps_n * sizeof(pagemap_t));
        for (size_t i = 0; i < child->pagemaps_n; i++) {
            child->pagemaps[i].virt = core->current_process->pagemaps[i].virt;
            child->pagemaps[i].n    = core->current_process->pagemaps[i].n;
            child->pagemaps[i].attr = core->current_process->pagemaps[i].attr;

            void * buf = alloc_pages(core->current_process->pagemaps[i].n);

            child->pagemaps[i].phys = virt_to_phys(buf);

            memcpy(buf, core->current_process->pagemaps[i].virt, core->current_process->pagemaps[i].n * PAGE_SIZE);
        }

        child->cwd = NULL;
        proc_set_cwd(child, core->current_process->cwd);

        child->sigqueue_sz = 0; // Pending signals are not inherited
    }

    if (core->current_process->to_sigreturn) {
        core->current_process->sighandling  = false;
        core->current_process->to_sigreturn = false;

        // Mark the stack as unused
        core->current_process->altstack.ss_flags &= ~(SS_ONSTACK);

        write_proc_regs(core->current_process, rf);
    }

    if (core->current_process->alarm && (pit0_ticks >= core->current_process->alarm)) {
        core->current_process->alarm = 0;
        push_proc_sig(core->current_process, SIGALRM);
    }

    // Check if there are signals to handle
    // Signals that are unhandleable, defaulted or ignored are handled immediately. Handlers are queued.
    size_t og_sigqueue_sz      = core->current_process->sigqueue_sz;
    size_t true_sigqueue_index = 0; // Elements in the queue may be deleted

    for (size_t i = 0; i < og_sigqueue_sz; i++) {
        int signum            = core->current_process->sigqueue[true_sigqueue_index]; // We cannot be sure this signal will be taken care of, so don't handle it
        struct sigaction * sa = get_proc_sigaction(core->current_process, signum);

        // Unavoidable signals
        if (signum == SIGKILL) {
            del_proc_sig(core->current_process, true_sigqueue_index);

            kill_process(core->current_process, UINT8_MAX);
            core->current_process = NULL;

            // Just run the scheduler over everything again
            schedule(rf);
            return;
        }

        // Is the signal masked?
        if (sigismember(&core->current_process->sigmask, signum)) {
            true_sigqueue_index++;
            continue;
        }

        // A signal has actually been caught
        core->current_process->pausing = false;

        if (!sa || ((uint64_t)sa->sa_handler == SIG_DFL)) {
            del_proc_sig(core->current_process, true_sigqueue_index);

            // Default handler
            switch (signum) {
                case SIGTERM: // Generally cause process termination
                case SIGINT:
                case SIGILL:
                case SIGFPE:
                case SIGSEGV:
                case SIGTRAP:
                    kill_process(core->current_process, UINT8_MAX);
                    core->current_process = NULL;

                    // Just run the scheduler over everything again
                    schedule(rf);
                    return;

                default:
                    break;
            }

        } else if ((uint64_t)sa->sa_handler == SIG_IGN) {
            del_proc_sig(core->current_process, true_sigqueue_index);

        } else if (!core->current_process->sighandling) {
            del_proc_sig(core->current_process, true_sigqueue_index);

            // A handler has been registered by the program and must now be jumped to
            core->current_process->sighandling  = true;
            core->current_process->to_sigreturn = false;

            // Just copy pretty much the entire context
            memcpy(&core->current_process->sigregs, &core->current_process->regs, sizeof(int_regframe_t));

            // If a program is running in usermode, all of its handlers are too!
            core->current_process->sigregs.cs = core->current_process->kmode ? (1*8) : ((6*8) | 3);
            core->current_process->sigregs.ss = core->current_process->kmode ? (2*8) : ((5*8) | 3);

            // Install the sigaltstack if demanded and possible
            if ((sa->sa_flags & SA_ONSTACK) && !(core->current_process->altstack.ss_flags & SS_DISABLE) && !(core->current_process->altstack.ss_flags & SS_ONSTACK)) {
                core->current_process->sigregs.rsp = (uint64_t)core->current_process->altstack.ss_sp \
                                             + core->current_process->altstack.ss_size;
                core->current_process->sigregs.rbp = core->current_process->sigregs.rsp;

                core->current_process->altstack.ss_flags |= SS_ONSTACK;
            }

            core->current_process->sigregs.rip = (uint64_t)sa->sa_handler;
            core->current_process->sigregs.rdi = signum;

            write_proc_regs(core->current_process, rf);

            // Other signals later in the queue will be handled too unless they require a handler (no break)
        } else {
            // Signal cannot be taken care of, advance to next one
            true_sigqueue_index++;
        }
    }
}

void proc_set_args(process_t * proc, int argc, char * argv[]) {
    // Copy the arguments
    if (argc && argv) {
        size_t len = 0;
        for (int i = 0; i < argc; i++) {
            len += strlen(argv[i]) + 1;
        }

        char ** new_argv = kmalloc(argc * sizeof(char*));
        char * new_buf = kmalloc(len);
        char * c = new_buf;

        for (int i = 0; i < argc; i++) {
            new_argv[i] = c;
            c = stpcpy(c, argv[i]) + 1;
        }

        proc->regs.rdi = proc->argc = argc;
        proc->regs.rsi = (uint64_t) (proc->argv = new_argv);

    } else {
        proc->regs.rdi = proc->argc = 0;
        proc->regs.rsi = (uint64_t) (proc->argv = NULL);
    }
}

process_t * add_process() {
    process_t * out = ll_push(processes);
    memset(out, 0, sizeof(process_t));
    out->pid = pid_counter++;
    out->sigactions = create_ll(sizeof(struct sigaction));
    sigemptyset(&out->sigmask);

    out->core = PROCESS_CORE_NONE;

    out->fds = create_ll(sizeof(fd_t));

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
    if (!IS_ALIVE(proc)) {
        kwarn(__FILE__,__func__,"tried to kill dead process");
        return;
    }

    // Free the memory
    free_pagemaps(proc->pagemaps, proc->pagemaps_n);
    kfree(proc->pagemaps);
    destroy_ll(proc->sigactions);

    proc->state = PROCESS_ZOMBIE;
    proc->exitcode = return_code;

    // Notify the parent that the child died
    if (proc->parent && get_proc_by_pid(proc->parent)) {
        process_t * parent = get_proc_by_pid(proc->parent);

        if (IS_ALIVE(parent)) {
            push_proc_sig(parent, SIGCHLD);

            // Check for SA_NOCLDWAIT (no zombie)
            struct sigaction * sa_sigchld = get_proc_sigaction(parent, SIGCHLD);
            if (sa_sigchld->sa_flags & SA_NOCLDWAIT)
                proc->state = PROCESS_NONE;
        }
    }

    // Close all FDs
    for (size_t i = 0; i < ll_len(proc->fds); i++) {
        fd_t * f = ll_get(proc->fds, i);
        if (!f->open)
            continue;
        fd_close(proc, f->n);
    }

    destroy_ll(proc->fds);
}

size_t proc_has_sig(process_t * proc, int signum) {
    // Check if a signal is in the signal queue.
    // Returns the signals HIGHEST (newest) index in the queue + 1,
    // or 0 if it is not present

    size_t out = 0;
    for (size_t i = 0; i < proc->sigqueue_sz; i++) {
        if (proc->sigqueue[i] == signum)
            out = i + 1;
    }

    return out;
}

int proc_set_cwd(process_t * proc, char * cwd) {
    // cwd should be absolute

    // Make sure the directory exists
    if (kexists(cwd) != FILE_DIR)
        return -1;

    size_t l = strlen(cwd);
    char * new_cwd = kmalloc(l + 1); // Null byte
    memcpy(new_cwd, cwd, l + 1);

    if (new_cwd[l - 1] != DIRSEP) { // Make sure a "/" is at the end
        new_cwd = krealloc(new_cwd, l + 2);
        new_cwd[l] = DIRSEP;
        new_cwd[l + 1] = 0;
    }

    if (clean_path(new_cwd) < 0)
        return -1;

    if (proc->cwd)
        kfree(proc->cwd);

    proc->cwd = new_cwd;

    return 0;
}

char * proc_get_cwd(process_t * proc) {
    if (proc->cwd)
        return proc->cwd;
    else
        return "/";
}

char * proc_to_abspath(process_t * proc, char * path) {
    // Essentially concatenation

    if (path[0] == DIRSEP) { // Already absolute? We need to copy it anyways
        size_t path_len = strlen(path);
        char * out = kmalloc(path_len + 1); // Null byte
        memcpy(out, path, path_len + 1);
        return out;
    }

    char * cwd = proc_get_cwd(proc);
    size_t cwd_len = strlen(cwd);
    size_t path_len = strlen(path);

    char * out = kmalloc(cwd_len + path_len + 1); // Null byte
    memcpy(out, cwd, cwd_len);
    memcpy(out + cwd_len, path, path_len + 1);

    return out; // Should be freed by the caller
}
