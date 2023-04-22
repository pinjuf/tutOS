#include "stddef.h"
#include "stdint.h"

extern uint64_t syscall(uint64_t syscall_num, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

void my_main();

__attribute__((noreturn))
void _start() {
    my_main();

    while (1);
}

void my_main() {
    uint64_t result = syscall(1, 2, 3, 4, 5, 6, 7);
    syscall(result, result, result, result, result, result, result);
}
