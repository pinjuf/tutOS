[BITS 64]

extern isr_noerr_exception, isr_err_exception, isr_default_int
global isr_stub_table, isr_default_stub

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

    mov rdi, %1       ; Exception number
    mov rsi, [rsp]    ; ERR
    mov rdx, [rsp+8]  ; RIP
    mov rcx, [rsp+16] ; CS
    mov r8,  [rsp+24] ; RFLAGS
    mov r9,  [rsp+32] ; RSP

    call isr_err_exception

    add rsp, 8 ; Pop ERR

    PIC_EOI

    iretq
%endmacro

%macro isr_noerr_stub 1
isr_stub_%+%1:
    cli

    mov rdi, %1       ; Exception number
    mov rsi, [rsp]    ; RIP
    mov rdx, [rsp+8]  ; CS
    mov rcx, [rsp+16] ; RFLAGS
    mov r8,  [rsp+24] ; RSP

    call isr_noerr_exception

    PIC_EOI

    iretq
%endmacro

isr_default_stub:
    mov rdi, 0xFF       ; Interrupt number (unknown, because this is the default stub)
    mov rsi, [rsp]      ; RIP
    mov rdx, [rsp+8]    ; CS
    mov rcx, [rsp+16]   ; RFLAGS
    mov r8,  [rsp+24]   ; RSP

    call isr_default_int

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

