#pragma once

#include "types.h"
#include "../../lib/unistd.h"
#include "../../lib/signal.h"

FILE * open(char * path, mode_t mode);
void close(FILE * file);
size_t read(FILE * file, void * buf, size_t size);
size_t write(FILE * file, void * buf, size_t size);
int64_t seek(FILE * file, int64_t offset, enum SEEKMODE mode);
pid_t fork();
int exec(char * file, char * argv[]);
void exit(int code);
pid_t waitpid(pid_t pid, int * status, size_t options);
pid_t getpid();
pid_t getppid();
size_t getdents(DIR * fd, dirent * dirp, size_t count);
dirent * readdir(DIR * d);
int pstat(char * pathname, stat * statbuf);
int fstat(FILE * fd, stat * statbuf);
int sigaction(int sig, struct sigaction * act);
int sigreturn();
int kill(pid_t pid, int sig);
int raise(int sig);
int signal(int sig, void (*func)(int));
int sigaltstack(stack_t * ss);
int pause();
uint64_t alarm(uint64_t seconds);
int sigprocmask(enum SIG_MASKHOW how, sigset_t * mask);
