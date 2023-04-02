[BITS 64]

global _kentry
extern _kmain

SECTION .kentry
_kentry:
    cli
    jmp _kmain
