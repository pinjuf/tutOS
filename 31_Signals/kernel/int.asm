[BITS 64]

extern isr_noerr_exception, isr_err_exception, isr_default_int, isr_irq0, isr_irq1, isr_irq5, isr_irq12, handle_syscall, isr_debugcall
global isr_stub_table, isr_default_stub, isr_irq0_stub, isr_irq1_stub, isr_irq5_stub, isr_irq12_stub, isr_syscall_stub, isr_debugcall_stub

; 512 FXSAVE bytes, plus 8 for alignment
%define PUSH_ALL_SIZE (18*8 + 512 + 8)

%macro PUSH_ALL 0
    push rbp

    mov rbp, rsp
    add rbp, 8     ; because we already pushed RBP

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

    mov rax, cr3
    push rax

    mov rax, 0x10
    mov ss, rax

    ; To save the FPU states, we need to be 16-byte aligned!
    ; We have already pushed 18 values ourselves, as well
    ; as 5 values from the interrupt itself. Therefore, we align
    ; ourselves with an extra 8 bytes.

    sub rsp, (512+8)
    fxsave [rsp]
%endmacro

%macro POP_ALL 0
    fxrstor [rsp]
    add rsp, (512+8)

    pop rax
    mov cr3, rax

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

    ; Because of the FPU's 16-byte alignment,
    ; we are in a bit of a predicament here,
    ; because now, the CPU pushed 6 values
    ; instead of 5, so we need to push a value
    ; for alignment once again

    sub rsp, 8 ; Alignment

    PUSH_ALL

    ; Notice the +8 alignment bytes
    mov rdi, %1                       ; Exception number
    mov rsi, [PUSH_ALL_SIZE+8+rsp]    ; ERR
    mov rdx, [PUSH_ALL_SIZE+8+rsp+8]  ; RIP
    mov rcx, [PUSH_ALL_SIZE+8+rsp+16] ; CS
    mov r8,  [PUSH_ALL_SIZE+8+rsp+24] ; RFLAGS
    mov r9,  [PUSH_ALL_SIZE+8+rsp+32] ; RSP
    mov r9,  [PUSH_ALL_SIZE+8+rsp+40] ; SS

    call isr_err_exception

    POP_ALL

    add rsp, 8 ; Pop alignment

    add rsp, 8 ; Pop ERR

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
    mov r9,  [PUSH_ALL_SIZE+rsp+32] ; SS

    call isr_noerr_exception

    POP_ALL

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
    mov r9,  [PUSH_ALL_SIZE+rsp+32]   ; SS

    call isr_default_int

    POP_ALL

    PIC_EOI

    iretq

isr_irq0_stub:
    PUSH_ALL

    mov rdi, rsp ; Register frame base address

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

isr_irq5_stub:
    PUSH_ALL

    call isr_irq5

    POP_ALL

    PIC_EOI
    iretq

isr_irq12_stub:
    PUSH_ALL

    call isr_irq12

    POP_ALL

    PIC_EOI
    iretq

isr_syscall_stub: ; Should probably NOT be used
    push rbp
    mov rbp, rsp

    push r9

    mov rcx, rdx
    mov rdx, rsi
    mov rsi, rdi
    mov rdi, rax
    mov r9, r8
    mov r8, r10

    call handle_syscall

    add rsp, 0x8

    pop rbp

    iretq

isr_debugcall_stub:
    PUSH_ALL

    mov rdi, rsp

    call isr_debugcall

    POP_ALL

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

