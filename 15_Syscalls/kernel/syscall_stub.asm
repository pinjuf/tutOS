[BITS 64]

global syscall_stub
extern handle_syscall

syscall_stub:
    ; The syscall triggerer is responsible for saving the registers, not the kernel

    mov [us_rsp], rsp ; Save user RSP & RBP
    mov [us_rbp], rbp

    mov rsp, 0x110000
    mov rbp, 0x110000

    push rcx ; Save passed on user RIP & RFLAGS
    push r11

    ; Linux
    ; RAX | RDI | RSI | RDX | R10 | R8  | R9
    ; N   | a0  | a1  | a2  | a3  | a4  | a5
    ; RDI | RSI | RDX | RCX | R8  | R9  | (stack)
    ; SysV ABI

    ; Juggle the arguments from Linux convention to SysV 64-bit calling convention

    push r9

    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    mov r9, r8
    mov r8, r10

    call handle_syscall

    add rsp, 0x8 ; pop arg5 (from r9)

    ; return value is in rax

    pop r11
    pop rcx

    mov rsp, [us_rsp]
    mov rbp, [us_rbp]

    o64 sysret

us_rsp: dq 0
us_rbp: dq 0
