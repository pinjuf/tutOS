#pragma once

#include "types.h"

#define TICKS_PER_SCHEDULE 4

// Structure describing the PUSH_ALL stack frame during an interrupt
typedef struct int_regframe_t {
    uint64_t cr3;
    uint64_t gs;
    uint64_t fs;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rbp;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) int_regframe_t;

typedef enum PROCESS_STATE {
    PROCESS_NONE = 0,
    PROCESS_NOT_RUNNING,
    PROCESS_RUNNING,
} PROCESS_STATE;

typedef struct pagemap_t {
    void * phys;
    void * virt;
    uint64_t attr;
    size_t n;
} pagemap_t;

typedef struct process_t {
    PROCESS_STATE state;
    pagemap_t * pagemaps;
    size_t pagemaps_n;
    int_regframe_t regs;
} process_t;

typedef uint16_t pid_t;

#define MAX_PROCESSES 256

extern process_t * processes;
extern process_t * current_process;

void init_scheduling(void);
void schedule(void * regframe_ptr);

#define PROC_PTR_TO_PID(ptr) (pid_t)(((uint64_t)ptr - (uint64_t)processes)/sizeof(process_t))
