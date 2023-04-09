[BITS 64]

extern isr_noerr_exception, isr_err_exception, isr_default_int, isr_irq0, isr_irq1, isr_irq12
global isr_stub_table, isr_default_stub, isr_irq0_stub, isr_irq1_stub, isr_irq12_stub

%define PUSH_ALL_SIZE (17*8)

%macro PUSH_ALL 0
    push rbp

    push rax
    push rbx
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    push rsi
    push rdi

    mov rax, fs
    push rax

    mov rax, gs
    push rax
%endmacro

%macro POP_ALL 0
    pop rax
    mov gs, rax

    pop rax
    mov fs, rax

    pop rdi
    pop rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rbx
    pop rax

    pop rbp
%endmacro

%macro PIC_EOI 0
    push rax
    mov al, 0x20
    out 0x20, al
    out 0xA0, al
    pop rax
%endmacro

%macro isr_err_stub 1
isr_stub_%+%1:
    cli

    PUSH_ALL

    mov rdi, %1                     ; Exception number
    mov rsi, [PUSH_ALL_SIZE+rsp]    ; ERR
    mov rdx, [PUSH_ALL_SIZE+rsp+8]  ; RIP
    mov rcx, [PUSH_ALL_SIZE+rsp+16] ; CS
    mov r8,  [PUSH_ALL_SIZE+rsp+24] ; RFLAGS
    mov r9,  [PUSH_ALL_SIZE+rsp+32] ; RSP

    call isr_err_exception

    POP_ALL

    add rsp, 8 ; Pop ERR

    PIC_EOI

    iretq
%endmacro

%macro isr_noerr_stub 1
isr_stub_%+%1:
    cli ; This is a trap gate

    PUSH_ALL

    mov rdi, %1                     ; Exception number
    mov rsi, [PUSH_ALL_SIZE+rsp]    ; RIP
    mov rdx, [PUSH_ALL_SIZE+rsp+8]  ; CS
    mov rcx, [PUSH_ALL_SIZE+rsp+16] ; RFLAGS
    mov r8,  [PUSH_ALL_SIZE+rsp+24] ; RSP

    call isr_noerr_exception

    POP_ALL

    PIC_EOI

    iretq
%endmacro

isr_default_stub:
    ; No cli needed, because this is an interrupt gate
    PUSH_ALL

    mov rdi, 0xFFFF                   ; Interrupt number (unknown, because this is the default stub)
    mov rsi, [PUSH_ALL_SIZE+rsp]      ; RIP
    mov rdx, [PUSH_ALL_SIZE+rsp+8]    ; CS
    mov rcx, [PUSH_ALL_SIZE+rsp+16]   ; RFLAGS
    mov r8,  [PUSH_ALL_SIZE+rsp+24]   ; RSP

    call isr_default_int

    POP_ALL

    PIC_EOI

    iretq

isr_irq0_stub:
    PUSH_ALL

    call isr_irq0

    POP_ALL

    PIC_EOI
    iretq

isr_irq1_stub:
    PUSH_ALL

    call isr_irq1

    POP_ALL

    PIC_EOI
    iretq

isr_irq12_stub:
    PUSH_ALL

    call isr_irq12

    POP_ALL

    PIC_EOI
    iretq

isr_stub_table:
%assign i 0
%rep 32
    dq isr_stub_%+i
    %assign i i+1
%endrep

isr_noerr_stub 0
isr_noerr_stub 1
isr_noerr_stub 2
isr_noerr_stub 3
isr_noerr_stub 4
isr_noerr_stub 5
isr_noerr_stub 6
isr_noerr_stub 7
isr_err_stub    8
isr_noerr_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_noerr_stub 15
isr_noerr_stub 16
isr_err_stub    17
isr_noerr_stub 18
isr_noerr_stub 19
isr_noerr_stub 20
isr_noerr_stub 21
isr_noerr_stub 22
isr_noerr_stub 23
isr_noerr_stub 24
isr_noerr_stub 25
isr_noerr_stub 26
isr_noerr_stub 27
isr_noerr_stub 28
isr_noerr_stub 29
isr_err_stub    30
isr_noerr_stub 31

