#pragma once

#include "types.h"
#include "../../lib/unistd.h"
#include "../../lib/signal.h"

int open(char * path, mode_t mode);
void close(int file);
size_t read(int file, void * buf, size_t size);
size_t write(int file, void * buf, size_t size);
int64_t seek(int file, int64_t offset, enum SEEKMODE mode);
pid_t fork();
int execve(char * file, char * argv[], char * envp[]);
void exit(int code);
pid_t waitpid(pid_t pid, int * status, size_t options);
pid_t getpid();
pid_t getppid();
size_t getdents(int fd, dirent * dirp, size_t count);
dirent * readdir(int d);
int pstat(char * pathname, stat * statbuf);
int fstat(int fd, stat * statbuf);
int sigaction(int sig, struct sigaction * act);
int sigreturn();
int kill(pid_t pid, int sig);
int raise(int sig);
int signal(int sig, void (*func)(int));
int sigaltstack(stack_t * ss);
int pause();
uint64_t alarm(uint64_t seconds);
int sigprocmask(enum SIG_MASKHOW how, sigset_t * mask);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int pipe(int fd[2]);
int getcwd(char * buf, size_t size);
int chdir(char * path);
int mount(char * source, char * target, char * filesystemtype, unsigned long mountflags, void * data);
int umount(char * target);
int mkdir(char * path);
int creat(char * path);
int unlink(char * path);
int rmdir(char * path);
