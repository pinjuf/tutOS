#pragma once

#include "types.h"
#include "unistd.h"

typedef struct dirent {
    enum FILETYPE d_type;
    size_t d_size;
    uint8_t d_namlen;
    char d_name[256];
} dirent;
