#pragma once

#include "types.h"

enum FILETYPE {
    FILE_UNKN,
    FILE_REG,
    FILE_DIR,
    FILE_BLK,
    FILE_DEV,
};

enum FILEMODE {
    FILE_R = 0,
    FILE_W,
};

enum SEEKMODE {
    SEEK_SET = 0,
    SEEK_CUR,
    SEEK_END,
};

// I know, I know, don't judge me...
typedef void FILE;
typedef void DIR;
typedef uint16_t pid_t;

FILE * open(char * path, enum FILEMODE mode);
void close(FILE * file);
size_t read(FILE * file, void * buf, size_t size);
size_t write(FILE * file, void * buf, size_t size);
int64_t seek(FILE * file, int64_t offset, enum SEEKMODE mode);
pid_t fork();
int exec(char * file);
void exit(int code);
