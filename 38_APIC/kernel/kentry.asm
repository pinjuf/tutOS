[BITS 64]

global _kentry
extern _kmain

SECTION .kentry
_kentry:
    cli

    ; Kernel stack
    mov rsp, 0x120000
    mov rbp, rsp

    jmp _kmain
