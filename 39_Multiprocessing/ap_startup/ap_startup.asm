[ORG 0xA000]
[BITS 16]

%include "../stage2/bpob.asm"

ap_start:
    cli

    jmp $

gdt:
    db 0, 0, 0, 0, 0, 0, 0, 0 ; Null descriptor
gdt_cs64:
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0b10011010, 0b10101111, 0x00 ; LM Code descriptor
gdt_end:

times 0x1000-($-$$) nop ; 1KiB of AP trampoline is a lot, but can be useful
