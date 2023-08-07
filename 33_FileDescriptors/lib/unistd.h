#pragma once

#include "types.h"

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

typedef struct stat {
    size_t st_dev; // device, a.k.a. mountpoint #N
    uint8_t st_mode; // only FILETYPE here
    size_t st_size;
} stat;

typedef uint16_t pid_t;

// wait4 options
#define WNOHANG    1
#define WUNTRACED  2
#define WCONTINUED 4

#define stdin  0
#define stdout 1
#define stderr 2
