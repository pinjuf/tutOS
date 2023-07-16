#pragma once

#include "types.h"
#include "signal.h"

enum FILETYPE {
    FILE_UNKN,
    FILE_REG,
    FILE_DIR,
    FILE_BLK,
    FILE_DEV,
};

typedef uint16_t mode_t;
#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR   3

enum SEEKMODE {
    SEEK_SET = 0,
    SEEK_CUR,
    SEEK_END,
};

typedef struct dirent {
    enum FILETYPE d_type;
    size_t d_size;
    uint8_t d_namlen;
    char d_name[256];
} dirent;

typedef struct {
    size_t st_dev; // device, a.k.a. mountpoint #N
    uint8_t st_mode; // only FILETYPE here
    size_t st_size;
} stat;

// I know, I know, don't judge me...
typedef void FILE;
typedef void DIR;
typedef uint16_t pid_t;

FILE * open(char * path, mode_t mode);
void close(FILE * file);
size_t read(FILE * file, void * buf, size_t size);
size_t write(FILE * file, void * buf, size_t size);
int64_t seek(FILE * file, int64_t offset, enum SEEKMODE mode);
pid_t fork();
int exec(char * file, char * argv[]);
void exit(int code);
pid_t waitpid(pid_t pid, int * status);
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
