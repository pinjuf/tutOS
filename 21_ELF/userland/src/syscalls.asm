[BITS 64]

global syscall

syscall:
    ; Might have stolen this from glibc (written during severe dehydration & malnourishment)
    mov rax, rdi
    mov r10, r8
    mov r8, r9
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    mov r9, [rsp+8]

    ;syscall
    int 0x80

    ret
