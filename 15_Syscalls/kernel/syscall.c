#include "syscall.h"

#include "util.h"

extern void syscall_stub(void);

void init_syscalls() {
    uint64_t star = (uint64_t)syscall_stub & 0xFFFFFFFF; // Handler's 32-bit EIP
    star |= (uint64_t)0x08 << 32;           // Kernel CS, Kernel SS - 8
    star |= (uint64_t)0x20 << 48;           // User CS - 16, User SS - 8
    wrmsr(STAR, star);

    wrmsr(LSTAR, (uint64_t)syscall_stub); // Handler's long mode RIP
    wrmsr(CSTAR, (uint64_t)syscall_stub); // Handler's IA32_COMP EIP
    wrmsr(SFMASK, 0x200);                 // RFLAGS Mask (disable IF)

    wrmsr(EFER, rdmsr(EFER) | 1); // SC (Syscall enable)
}

uint64_t handle_syscall(uint64_t n, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    switch (n) { // TODO: Add more syscalls
        default: {
                     kputs("SYSCALL n=0x");
                     kputhex(n);
                     kputs(" arg0=0x");
                     kputhex(arg0);
                     kputs(" arg1=0x");
                     kputhex(arg1);
                     kputs(" arg2=0x");
                     kputhex(arg2);
                     kputs(" arg3=0x");
                     kputhex(arg3);
                     kputs(" arg4=0x");
                     kputhex(arg4);
                     kputs(" arg5=0x");
                     kputhex(arg5);
                     kputc('\n');
                 }
    }

    return 0;
}
