#pragma once

#include "types.h"

#define TICKS_PER_SCHEDULE 4

typedef uint16_t pid_t;

typedef struct fpu_fxsave_t {
    uint16_t fcw;
    uint16_t fsw;
    uint8_t ftw;
    uint8_t res0;
    uint16_t fop;
    uint32_t fpu_ip;
    uint16_t cs;
    uint16_t res1;
    uint32_t fpu_dp;
    uint16_t ds;
    uint16_t res2;
    uint32_t mxcsr;
    uint32_t mxcsr_mask;
    uint8_t st_mm[8][16]; // Only bits 0..80 are used
    uint8_t xmm[16][16];
    uint8_t res3[48];
    uint8_t unused0[48];
} __attribute__((packed)) fpu_fxsave_t;

// Structure describing the PUSH_ALL stack frame during an interrupt
typedef struct int_regframe_t {
    fpu_fxsave_t fpu;
    uint8_t alignment[8];
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

    void * stack_heap;  // The position of the stack in heap memory (it is mapped to sth like ELF_DEF_RSP)
    size_t stack_pages;

    bool to_fork;
    pid_t latest_child;

    pagemap_t * pagemaps;
    size_t pagemaps_n;

    int_regframe_t regs;
} process_t;

#define MAX_PROCESSES 256

extern process_t * processes;
extern process_t * current_process;
extern bool do_scheduling;

void init_scheduling(void);
void schedule(void * regframe_ptr);

#define PROC_PTR_TO_PID(ptr) (pid_t)(((uint64_t)ptr - (uint64_t)processes)/sizeof(process_t))
