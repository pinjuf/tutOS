#include "unistd.h"
#include "syscall.h"
#include "dirent.h"
#include "stdlib.h"

FILE * open(char * path, enum FILEMODE mode) {
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

pid_t waitpid(pid_t pid) {
    return syscall(61, pid, 0, 0, 0, 0, 0);
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
