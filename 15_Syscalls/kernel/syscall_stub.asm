[BITS 64]

global syscall_stub
extern handle_syscall

syscall_stub:
    mov [us_rsp], rsp ; Save user RSP & RBP
    mov [us_rbp], rbp

    mov rsp, 0x110000
    mov rbp, 0x110000

    push rcx ; Save passed on user RIP & RFLAGS
    push r11

    ; Args are passed like on Linux:
    ; rax=num, rdi=arg0, rsi=arg1, rdx, r10, r8, r9

    ; Juggle the arguments from Linux convention to SysV 64-bit calling convention
    push r9  ; arg5
    push r8  ; arg4
    push r10 ; arg3
    push rdx ; arg2
    push rsi ; arg1
    push rdi ; arg0
    push rax ; n

    pop rdi  ; n
    pop rsi  ; arg0
    pop rdx  ; arg1
    pop rcx  ; arg2
    pop r8   ; arg3
    pop r9   ; arg4
    ; arg5 is on the stack

    call handle_syscall

    add rsp, 0x8 ; pop arg5

    ; return value is in rax

    pop r11
    pop rcx

    mov rsp, [us_rsp]
    mov rbp, [us_rbp]

    o64 sysret

us_rsp: dq 0
us_rbp: dq 0
