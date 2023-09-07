[ORG 0xA000]
[BITS 16]

%include "../stage2/bpob.asm"

ap_start:
    cli

    ; Setup the A20 line
    in al, 0x92
    or al, 0b10
    out 0x92, al

    lgdt [gdtr]

    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp 0x08:ap_start_prot
[BITS 32]
ap_start_prot:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Init the x87 FPU
    finit

    ; Enable SSE etc.
    mov eax, cr0
    and ax, ~(1 << 2) ; Disable CR0.EM
    or  ax,  1 << 1   ; Enable  CR0.MP
    mov cr0, eax

    mov eax, cr4
    or ax, 0b11 << 9 ; Enable CR4.OSFXSR and CR4.OSXMMEXCPT
    mov cr4, eax

    mov edi, 0x7D000 ; We use the BSP's stack for now
    mov cr3, edi

    mov eax, cr4
    or eax, 1 << 4 | 1 << 5 | 1 << 7 ; Enable PSE, PAE and PGE
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8 ; LM bit
    wrmsr

    mov eax, cr0
    or eax, 1 << 31 ; Paging enable
    mov cr0, eax

    jmp 0x20:ap_start_lm

[BITS 64]
ap_start_lm:
    xor eax, eax ; Load mode only needs CS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov qword rsp, [BPOB + bpob.ap_stack]
    mov rbp, rsp

    inc byte [BPOB + bpob.ap_count]

    jmp [BPOB + bpob.ap_entry]

gdtr:
    dw gdt_end - gdt - 1 ; Size (-1)
    dd gdt               ; Offset

gdt:
    db 0, 0, 0, 0, 0, 0, 0, 0 ; Null descriptor
gdt_cs:
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0b10011010, 0b11001111, 0x00 ; Code descriptor
gdt_ds:
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0b10010010, 0b11001111, 0x00 ; Data descriptor
gdt_ss:
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0b10010110, 0b11001111, 0x00 ; Stack descriptor
gdt_cs64:
    db 0xFF, 0xFF, 0x00, 0x00, 0x00, 0b10011010, 0b10101111, 0x00 ; LM Code descriptor
gdt_end:

times 0x1000-($-$$) nop ; 1KiB of AP trampoline is a lot, but can be useful
