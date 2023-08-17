#include "syscall.h"
#include "vfs.h"
#include "util.h"
#include "schedule.h"
#include "mm.h"
#include "elf.h"
#include "signal.h"
#include "isr.h"
#include "main.h"
#include "fd.h"
#include "pipe.h"

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
            int fd            = arg0;
            void * buf        = (void*)arg1;
            size_t count      = arg2;

            sti; // For keyboard read etc.
            return fd_read(current_process, fd, buf, count);
        }
        case 1: { // write
            int fd            = arg0;
            void * buf        = (void*)arg1;
            size_t count      = arg2;

            return fd_write(current_process, fd, buf, count);
        }
        case 2: { // open
            char * path = (void*)arg0;
            mode_t mode = arg1;

            path = proc_to_abspath(current_process, path);

            filehandle_t * fh = kopen(path, mode);

            kfree(path); // Needs to be done after abspath()

            if (!fh)
                return -1;

            fd_t * new_fd = add_fd(current_process);
            new_fd->type = FD_VFS;
            new_fd->handle = fh;
            fh->fd_refs++;

            return new_fd->n;
        }
        case 3: { // close
            int fd = arg0;

            return fd_close(current_process, fd);
        }
        case 4: { // stat
            char * path = (void*)arg0;
            stat * out  = (void*)arg1;

            path = proc_to_abspath(current_process, path);

            // Please don't judge me
            filehandle_t * fh = kopen(path, 0);

            kfree(path);

            if (!fh)
                fh = kopen(path, O_WRONLY);
            if (!fh)
                return 1;

            fh_to_stat(fh, out);

            return 0;
        }
        case 5: { // fstat
            int fd     = arg0;
            stat * out = (void*)arg1;

            fd_stat(current_process, fd, out);

            return 0;
        }
        case 8: { // seek
            int fd             = arg0;
            int64_t offset     = arg1;
            enum SEEKMODE mode = arg2;

            fd_t * fd_s = get_proc_fd(current_process, fd);

            // Only files can be seek'd
            if (fd_s->type != FD_VFS)
                return -1;

            filehandle_t * fh = fd_s->handle;

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
        case 14: { // sigprocmask
            enum SIG_MASKHOW how = arg0;
            sigset_t * set       = (void*)arg1;
            // This technically a legacy syscall that doesn't need a setsize (however, the setsize is limited)

            switch (how) {
                case SIG_BLOCK:
                    for (uint32_t i = 0; i < set->sig_n; i++) {
                        if (sigaddset(&current_process->sigmask, set->sigs[i]))
                            return -1;
                    }
                    break;

                case SIG_UNBLOCK:
                    for (uint32_t i = 0; i < set->sig_n; i++) {
                        sigdelset(&current_process->sigmask, set->sigs[i]);
                    }
                    break;

                case SIG_SETMASK:
                    memcpy(&current_process->sigmask, set, sizeof(sigset_t));
                    break;

                default:
                    return -1;
            }

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
        case 22: { // pipe
            int * fds = (int*)arg0;

            pipe_t * pipe = mkpipe(SYS_PIPESZ);

            fd_t * in  = add_fd(current_process);
            pipe->head_fds++;
            in->type = FD_PIPE_I;
            in->handle = pipe;

            fd_t * out = add_fd(current_process);
            pipe->tail_fds++;
            out->type = FD_PIPE_O;
            out->handle = pipe;

            fds[0] = in->n;
            fds[1] = out->n;

            return 0;
        }
        case 32: { // dup
            int fd = arg0;

            fd_t * old_fd_s = get_proc_fd(current_process, fd);
            if (!old_fd_s)
                return -1;

            fd_t * new_fd_s = add_fd(current_process);
            int new_fd = new_fd_s->n;

            memcpy(new_fd_s, old_fd_s, sizeof(fd_t));
            new_fd_s->n = new_fd; // got overwritten by memcpy

            switch (new_fd_s->type) {
                case FD_VFS: {
                    filehandle_t * fh = new_fd_s->handle;
                    fh->fd_refs++;
                    break;
                }
                case FD_PIPE_I: {
                    pipe_t * pipe = new_fd_s->handle;
                    pipe->head_fds++;
                    break;
                }
                case FD_PIPE_O: {
                    pipe_t * pipe = new_fd_s->handle;
                    pipe->tail_fds++;
                    break;
                }
                default:
                    return -1;
            }

            return new_fd;
        }
        case 33: { // dup2
            int oldfd = arg0;
            int newfd = arg1;

            if (oldfd == newfd)
                return -1;

            fd_t * old_fd_s = get_proc_fd(current_process, oldfd);
            if (!old_fd_s)
                return -1;

            fd_t * new_fd_s = get_proc_fd(current_process, newfd);
            if (new_fd_s) { // Close it if it already exists
                fd_close(current_process, newfd);
            }
            new_fd_s = add_fd(current_process); // Inc's p->fd_n, but that shouldn't matter

            memcpy(new_fd_s, old_fd_s, sizeof(fd_t));
            new_fd_s->n = newfd;

            switch (new_fd_s->type) {
                case FD_VFS: {
                    filehandle_t * fh = new_fd_s->handle;
                    fh->fd_refs++;
                    break;
                }
                case FD_PIPE_I: {
                    pipe_t * pipe = new_fd_s->handle;
                    pipe->head_fds++;
                    break;
                }
                case FD_PIPE_O: {
                    pipe_t * pipe = new_fd_s->handle;
                    pipe->tail_fds++;
                    break;
                }
                default:
                    return -1;
            }

            return newfd;
        }
        case 34: { // pause
            sti;

            current_process->pausing = true;
            while (current_process->pausing);

            cli;

            return 0;
        }
        case 37: { // alarm
            uint64_t seconds = arg0;

            // When would the next alarm have been? (rounded upwards to seconds)
            uint64_t old_seconds;
            if (current_process->alarm)
                old_seconds = (current_process->alarm - pit0_ticks + PIT0_FREQ - 1) / PIT0_FREQ;
            else
                old_seconds = 0;

            if (seconds)
                current_process->alarm = pit0_ticks + seconds * PIT0_FREQ;
            else // seconds = 0 means to cancel any alarm
                current_process->alarm = 0;

            return old_seconds;
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
        case 59: { // execve
            char * file = (char*)arg0;
            char ** argv = (char**)arg1;
            // TODO: Implement environment vars

            int argc = 0;
            if (argv)
                for (size_t i = 0; argv[i]; i++)
                    argc++;

            file = proc_to_abspath(current_process, file);

            filehandle_t * elf_handle = kopen(file, O_RDONLY);

            kfree(file);

            if (!elf_handle) {
                kwarn(__FILE__,__func__,"exec file not found");
                return 1;
            }

            if (elf_handle->type != FILE_REG) {
                kwarn(__FILE__,__func__,"exec file not regular file");
                return 1;
            }

            // We cannot be sure elf_load will succeed, but if it does, it will overwrite the pagemaps
            ll_head * original_sa   = current_process->sigactions;
            pagemap_t * original_p  = current_process->pagemaps;
            size_t original_pn      = current_process->pagemaps_n;

            void * elf_buf = kmalloc(elf_handle->size);
            kread(elf_handle, elf_buf, elf_handle->size);
            kclose(elf_handle);

            int status = elf_load(current_process, elf_buf, 0x10, false); // 64 KiB stack
            if (status) {
                return 1;
            }

            free_pagemaps(original_p, original_pn);
            kfree(original_p);
            destroy_ll(original_sa);

            proc_set_args(current_process, argc, argv);
            kfree(elf_buf);

            // Context changes don't pass signals on
            current_process->sigactions  = create_ll(sizeof(struct sigaction));
            current_process->sigqueue_sz = 0;
            current_process->sighandling = false;

            memset(&current_process->altstack, 0, sizeof(stack_t));

            current_process->to_exec = true;

            sti; // Wait for the scheduler to pick us up
            while (1);
        }
        case 60: { // exit
            uint8_t return_code = arg0 & 0xFF;

            kill_process(current_process, return_code);

            current_process = NULL; // The scheduler will see this and pick us up

            sti;
            while (1);
        }
        case 61: { // wait4
            pid_t pid = arg0;
            int * status = (void*)arg1;
            size_t options = arg2;

            process_t * proc = get_proc_by_pid(pid);
            if (!proc)
                return -1;

            if (options & WNOHANG) {
                if (!IS_ALIVE(proc)) {
                    if (status)
                        *status = proc->exitcode;

                    if ((current_process->pid == proc->parent) && (current_process->state == PROCESS_ZOMBIE))
                        proc->state = PROCESS_NONE;

                    return pid;
                } else {
                    return 0;
                }
            }

            volatile PROCESS_STATE orig = proc->state;

            sti;

            while (IS_ALIVE(proc) \
                || ((options & WUNTRACED)  && (orig == PROCESS_RUNNING) && (proc->state != PROCESS_STOPPED)) \
                || ((options & WCONTINUED) && (orig == PROCESS_STOPPED) && (proc->state != PROCESS_RUNNING)));

            cli;

            if (status)
                *status = proc->exitcode;

            if ((current_process->pid == proc->parent) && (current_process->state == PROCESS_ZOMBIE))
                proc->state = PROCESS_NONE;

            return pid;
        }
        case 62: { // kill
            pid_t pid = arg0;
            int   sig = arg1;

            process_t * target = get_proc_by_pid(pid);
            if (!target)
                return -1;

            if (!IS_ALIVE(target))
                return -1;

            push_proc_sig(target, sig);

            return 0;
        }
        case 78: { // getdents (technically getdents64)
            int fd       = arg0;
            dirent * out = (void*)arg1;
            size_t count = arg2;

            fd_t * fd_struct = get_proc_fd(current_process, fd);
            if (!fd_struct)
                return -1;

            if (fd_struct->type != FD_VFS)
                return -1;

            filehandle_t * dir = fd_struct->handle;

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
        case 79: { // getcwd
            char * buf = (void*)arg0;
            size_t size = arg1;

            if (size < strlen(current_process->cwd) + 1)
                return -1;

            char * cwd = proc_get_cwd(current_process);
            strcpy(buf, cwd);

            return 0;
        }
        case 80: { // chdir
            char * path = (void*)arg0;

            path = proc_to_abspath(current_process, path);

            int status = proc_set_cwd(current_process, path);

            kfree(path);

            return status;
        }
        case 110: { // getppid
            return current_process->parent;
        }
        case 131: { // sigaltstack
            struct stack_t * altstack = (void*)arg0;

            memcpy(&current_process->altstack, altstack, sizeof(struct stack_t));

            return 0;
        }
        case 165: { // mount
            char * source = (void*)arg0;
            char * target = (void*)arg1;
            char * type   = (void*)arg2;
            uint64_t flags = arg3;
            void * data   = (void*)arg4;

            (void)flags; // TODO, implement when necessary
            (void)data;

            if (source)
                source = proc_to_abspath(current_process, source);
            target = proc_to_abspath(current_process, target);

            int status = mount(source, target, type);

            kfree(source);
            kfree(target);

            return status;
        }
        case 166: { // umount
            char * target = (void*)arg0;

            target = proc_to_abspath(current_process, target);

            int status = unmount(target);

            kfree(target);

            return status;
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

            push_proc_sig(current_process, SIGSEGV);

            kprintf(" < SYSCALL UNKN n=%d "
                  " arg0=0x%x arg1=0x%x arg2=0x%x arg3=0x%x arg4=0x%x arg5=0x%x >\n",
                  n, arg0, arg1, arg2, arg3, arg4, arg5);

            cli;
        }
    }

    return -1; // The one true meaning of life, the universe, and everything (and also 66 in decimal)
}
