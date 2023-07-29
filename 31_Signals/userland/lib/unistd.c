#include "unistd.h"
#include "syscall.h"
#include "dirent.h"
#include "stdlib.h"
#include "signal.h"

FILE * open(char * path, mode_t mode) {
    return (void*) syscall(2, (uint64_t)path, (size_t)mode, 0, 0, 0, 0);
}

void close(FILE * file) {
    syscall(3, (uint64_t)file, 0, 0, 0, 0, 0);
}

size_t read(FILE * file, void * buf, size_t count) {
    return syscall(0, (uint64_t)file, (uint64_t)buf, count, 0, 0, 0);
}

size_t write(FILE * file, void * buf, size_t count) {
    return syscall(1, (uint64_t)file, (uint64_t)buf, count, 0, 0, 0);
}

int64_t seek(FILE * file, int64_t offset, enum SEEKMODE mode) {
    return syscall(8, (uint64_t)file, offset, (uint64_t)mode, 0, 0, 0);
}

pid_t fork() {
    return syscall(57, 0, 0, 0, 0, 0, 0);
}

int exec(char * file, char * argv[]) {
    return syscall(59, (uint64_t)file, (uint64_t)argv, 0, 0, 0, 0);
}

void exit(int code) {
    syscall(60, code, 0, 0, 0, 0, 0);
    __builtin_unreachable();
}

pid_t waitpid(pid_t pid, int * status, size_t options) {
    return syscall(61, pid, (uint64_t)status, options, 0, 0, 0);
}

pid_t getpid() {
    return syscall(39, 0, 0, 0, 0, 0, 0);
}

pid_t getppid() {
    return syscall(110, 0, 0, 0, 0, 0, 0);
}

size_t getdents(DIR * fd, dirent * dirp, size_t count) {
    return syscall(78, (uint64_t)fd, (uint64_t)dirp, count, 0, 0, 0);
}

dirent * readdir(DIR * d) {
    dirent * out = malloc(sizeof(dirent));

    size_t read = getdents(d, out, 1);

    if (read == 0)
        return NULL;

    return out;
}

int pstat(char * pathname, stat * statbuf) {
    // I know it should be called stat(), but I don't wanna
    // fight GCC again...
    return syscall(4, (uint64_t)pathname, (uint64_t)statbuf, 0, 0, 0, 0);
}

int fstat(FILE * fd, stat * statbuf) {
    return syscall(5, (uint64_t)fd, (uint64_t)statbuf, 0, 0, 0, 0);
}

int sigaction(int sig, struct sigaction * act) {
    return syscall(13, (uint64_t)sig, (uint64_t)act, 0, 0, 0, 0);
}

int sigreturn() {
    return syscall(15, 0, 0, 0, 0, 0, 0);
}

int kill(pid_t pid, int sig) {
    return syscall(62, pid, sig, 0, 0, 0, 0);
}

int raise(int sig) {
    return kill(getpid(), sig);
    // note: it might be wise here to wait until the next jiffy
    // to make sure the handler gets executed before this function returns
}

int signal(int sig, void (*func)(int)) {
    struct sigaction sa;
    sa.sa_handler = func;
    sa.sa_flags   = 0;
    return sigaction(sig, &sa);
}

int sigaltstack(stack_t * ss) {
    return syscall(131, (uint64_t)ss, 0, 0, 0, 0, 0);
}

int pause() {
    return syscall(34, 0, 0, 0, 0, 0, 0);
}

uint64_t alarm(uint64_t seconds) {
    return syscall(37, seconds, 0, 0, 0, 0, 0);
}
