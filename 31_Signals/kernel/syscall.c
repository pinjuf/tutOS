#include "syscall.h"
#include "vfs.h"
#include "util.h"
#include "schedule.h"
#include "mm.h"
#include "elf.h"
#include "signal.h"

extern void syscall_stub(void);

void init_syscalls() {
    uint64_t star = (uint64_t)syscall_stub & 0xFFFFFFFF; // Handler's 32-bit EIP
    star |= (uint64_t)0x08 << 32;           // Kernel CS, Kernel SS - 8
    star |= (uint64_t)0x20 << 48;           // User CS - 16, User SS - 8
    wrmsr(STAR, star);

    wrmsr(LSTAR, (uint64_t)syscall_stub); // Handler's long mode RIP
    wrmsr(CSTAR, (uint64_t)syscall_stub); // Handler's IA32_COMP EIP
    wrmsr(SFMASK, 0x200);                 // Handler's RFLAGS mask

    wrmsr(EFER, rdmsr(EFER) | 1); // SC (Syscall enable)
}

uint64_t handle_syscall(uint64_t n, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    // If we are doing sth that takes time and can be interrupted, we should sti and cli later

    switch (n) {
        case 0: { // read
            filehandle_t * fh = (void*)arg0;
            void * buf        = (void*)arg1;
            size_t count      = arg2;

            sti; // For keyboard read etc.
            return kread(fh, buf, count);
        }
        case 1: { // write
            filehandle_t * fh = (void*)arg0;
            void * buf        = (void*)arg1;
            size_t count      = arg2;

            return kwrite(fh, buf, count);
        }
        case 2: { // open
            char * path = (void*)arg0;
            mode_t mode = arg1;

            return (size_t)kopen(path, mode);
        }
        case 3: { // close
            filehandle_t * fh = (void*)arg0;

            kclose(fh);

            return 0;
        }
        case 4: { // stat
            char * filename = (void*)arg0;
            stat * out      = (void*)arg1;

            // Please don't judge me
            filehandle_t * fh = kopen(filename, O_RDONLY);
            if (!fh)
                fh = kopen(filename, O_WRONLY);
            if (!fh)
                return 1;

            fh_to_stat(fh, out);

            return 0;
        }
        case 5: { // fstat
            filehandle_t * handle = (void*)arg0;
            stat * out            = (void*)arg1;

            fh_to_stat(handle, out);

            return 0;
        }
        case 8: { // seek
            filehandle_t * fh   = (void*)arg0;
            int64_t offset      = arg1;
            enum SEEKMODE mode  = arg2;

            switch (mode) {
                case SEEK_SET:
                    fh->curr = offset;
                    break;
                case SEEK_CUR:
                    fh->curr += offset;
                    break;
                case SEEK_END: // I cannot imagine a universe where this is truly needed
                    fh->curr = fh->size + offset;
                    break;
            }

            return fh->curr;
        }
        case 13: { // sigaction
            int signum                = arg0;
            struct sigaction * action = (void*)arg1;

            action->sa_sig = signum;

            register_sigaction(current_process, action);

            return 0;
        }
        case 15: { // sigreturn
            if (!current_process->sighandling)
                return -1;

            current_process->to_sigreturn = true;

            // Scheduler will see the flag and pick us up
            sti;
            while (1);
        }
        case 39: { // getpid
            return current_process->pid;
        }
        case 57: { // fork
            current_process->to_fork = true;

            volatile process_t * original_process = current_process;

            sti;
            while (current_process->to_fork);
            cli;

            if (original_process != current_process)
                return 0;
            return current_process->latest_child;
        }
        case 59: { // exec
            char * file = (char*)arg0;
            char ** argv = (char**)arg1;

            int argc = 0;
            if (argv)
                for (size_t i = 0; argv[i]; i++)
                    argc++;

            filehandle_t * elf_handle = kopen(file, O_RDONLY);
            if (!elf_handle) {
                kwarn(__FILE__,__func__,"exec file not found");
                return 1;
            }

            // We cannot be sure elf_load will succeed, but if it does, it will overwrite the pagemaps
            size_t original_pn    = current_process->pagemaps_n;
            ll_head * original_sa = current_process->sigactions;
            pagemap_t * original_p  = current_process->pagemaps;

            void * elf_buf = kmalloc(elf_handle->size);
            kread(elf_handle, elf_buf, elf_handle->size);
            kclose(elf_handle);

            int status = elf_load(current_process, elf_buf, 0x10, false); // 64 KiB stack
            if (status) {
                return 1;
            }

            for (size_t i = 0; i < original_pn; i++) {
                // Dirty, I know...
                uint64_t addr = (uint64_t)original_p[i].phys;
                addr -= HEAP_PHYS;
                addr += HEAP_VIRT;
                free_pages((void*)addr, original_p[i].n);
            }
            kfree(original_p);
            destroy_ll(original_sa);

            proc_set_args(current_process, argc, argv);
            kfree(elf_buf);

            // Context changes don't pass signals on
            current_process->sigactions  = create_ll(sizeof(struct sigaction));
            current_process->sigqueue_sz = 0;
            current_process->sighandling = false;

            current_process->to_exec = true;

            sti; // Wait for the scheduler to pick us up
            while (1);
        }
        case 60: { // exit
            uint8_t return_code = arg0 & 0xFF;

            // Free the memory
            for (size_t i = 0; i < current_process->pagemaps_n; i++) {
                uint64_t addr = (uint64_t)current_process->pagemaps[i].phys;
                addr -= HEAP_PHYS;
                addr += HEAP_VIRT;
                free_pages((void*)addr, current_process->pagemaps[i].n);
            }
            kfree(current_process->pagemaps);
            destroy_ll(current_process->sigactions);

            current_process->state = PROCESS_ZOMBIE;
            current_process->exitcode = return_code;

            // Notify the parent that the child died
            // TODO: Check for sa_flags
            if (current_process->parent && get_proc_by_pid(current_process->parent)) {
                push_proc_sig(get_proc_by_pid(current_process->parent), SIGCHLD);
            }

            current_process = NULL; // The scheduler will see this and pick us up

            sti;
            while (1);
        }
        case 61: { // wait4
            pid_t pid = arg0;
            int * status = (void*)arg1;

            process_t * proc = get_proc_by_pid(pid);
            if (!proc)
                return -1;

            sti;
            while (proc->state != PROCESS_ZOMBIE);
            cli;

            *status = proc->exitcode;

            if (current_process->pid == proc->parent)
                proc->state = PROCESS_NONE;

            return pid;
        }
        case 78: { // getdents (technically getdents64)
            filehandle_t * dir = (void*)arg0;
            dirent * out       = (void*)arg1;
            size_t count       = arg2;

            size_t read = 0;

            for (size_t i = 0; i < count; i++) {
                dirent * o = kreaddir(dir);
                if (o == NULL)
                    return 0; // End of directory

                memcpy(&out[i], o, sizeof(*o));
                read += sizeof(*o);
            }

            return read;
        }
        case 110: { // getppid
            return current_process->parent;
        }
        case 336: { // malloc
            size_t n = arg0;
            return (uint64_t)kmalloc(n);
        }
        case 337: { // free
            void * addr = (void*) arg0;
            kfree(addr);
            return 0;
        }

        default: {
            sti;
            kputs("SYSCALL n=0x");
            kputhex(n);
            kputs(" arg0=0x");
            kputhex(arg0);
            kputs(" arg1=0x");
            kputhex(arg1);
            kputs(" arg2=0x");
            kputhex(arg2);
            kputs(" arg3=0x");
            kputhex(arg3);
            kputs(" arg4=0x");
            kputhex(arg4);
            kputs(" arg5=0x");
            kputhex(arg5);
            kputc('\n');
            cli;
        }
    }

    return -1; // The one true meaning of life, the universe, and everything (and also 66 in decimal)
}
