#pragma once

#include "types.h"
#include "schedule.h"
#include "signal.h"

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss);

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss);

void isr_default_int(uint16_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss);

void isr_irq0(int_regframe_t * regframe);
void isr_irq1(void);
void isr_irq12(void);

void isr_debugcall(int_regframe_t * regframe);

extern uint64_t pit0_ticks;

// All x86 standard ISA exceptions, error codes are marked with X
enum X86_EXCEPTION {
    EXC_DE = 0,    //   Division Error (Zero Division)
    EXC_DB,        //   Debug
    EXC_NMI,       //   Non Maskable Interrupt
    EXC_BP,        //   Breakpoint
    EXC_OF,        //   Overflow
    EXC_BR,        //   Bound Range Exceeded
    EXC_UD,        //   Invalid Opcode
    EXC_NM,        //   Device Not Available
    EXC_DF,        // X Double Fault
    EXC_CSO,       //   Coprocessor Segment Overrun
    EXC_TS,        // X Invalid TSS
    EXC_NP,        // X Segment Not Present
    EXC_SS,        // X Stack Segment Fault
    EXC_GP,        // X General Protection Fault
    EXC_PF,        // X Page Fault
    // reserved
    EXC_MF = 16,   //   x87 Floating-Point Exception
    EXC_AC,        // X Alignment Check
    EXC_MC,        //   Machine Check
    EXC_XM,        //   SIMD Floating-Point Exception
    EXC_VE,        //   Virtualization Exception
    EXC_CP,        // X Control Protection Exception
    // reserved
    // reserved
    // reserved
    // reserved
    // reserved
    // reserved
    EXC_HV = 28,   //   Hypervisor Injection Exception
    EXC_VC,        // X VMM Communication Exception
    EXC_SX,        // X Security Exception
};

// What SIGNAL is to be sent to a process when it causes an exception
static const enum SIGNAL EXCEPTION_SIGNALS[] = {
    SIGFPE,
    SIGTRAP,
    SIGKILL,
    SIGTRAP,
    SIGKILL,
    SIGKILL,
    SIGILL,
    SIGKILL,
    SIGKILL,
    SIGFPE,
    SIGKILL,
    SIGSEGV,
    SIGSEGV,
    SIGILL,
    SIGSEGV,
    SIGKILL,
    SIGFPE,
    SIGKILL,
    SIGKILL,
    SIGFPE,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
    SIGKILL,
};
