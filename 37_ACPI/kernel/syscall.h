#pragma once

#include "types.h"
#include "vfs.h"

#define EFER   0xC0000080
#define STAR   0xC0000081
#define LSTAR  0xC0000082
#define CSTAR  0xC0000083
#define SFMASK 0xC0000084

#define SYS_PIPESZ 0x2000 // Default pipe size

void init_syscalls();
uint64_t handle_syscall(uint64_t n, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

// Some syscalls that need proper functions
int sys_open(char * path, mode_t mode);
