[ORG 0x9000]
[BITS 16]

stage2_start:
    cli

    lgdt [gdtr] ; Load the GDT
    
    mov eax, cr0
    or al, 1     ; Set the protection enable bit
    mov cr0, eax

    jmp 0x08:reload_cs ; CS needs to be set this way

[BITS 32]
reload_cs:
    mov ax, 0x10 ; Set all of our selectors using GDT offsets
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ax, 0x18
    mov ss, ax
    mov esp, 0x9000

    ; Step 1: Check if we even have Long Mode
    mov eax, 0x80000001 ; Extended CPUID
    cpuid
    test edx, 1 << 29   ; LM Available bit
    jz no_lm

    ; Step 2: Setup paging, directly below the EBDA

    ; PML4T : 0x7D000 - 0x7DFFF
    ; PDPT  : 0x7E000 - 0x7EFFF
    ; PDT   : 0x7F000 - 0x7FFFF
    ; PT    : (using size bit in the PDT)

    mov edi, 0x7D000
    mov cr3, edi
    xor eax, eax
    mov ecx, 0xC00
    rep stosd
    mov edi, cr3

    mov DWORD [edi], 0x7E000 | 0b11
    add edi, 0x1000
    mov DWORD [edi], 0x7F000 | 0b11
    add edi, 0x1000
    mov DWORD [edi], 0b11 | 1<<7    ; P, RW, PS
    add edi, 8
    mov DWORD [edi], 0x200000 | 0b11 | 1<<7

    ; Step 3: Enable PSE and PAE
    mov eax, cr4
    or eax, 1 << 4 | 1 << 5
    mov cr4, eax

    ; Step 4: Enable Long Mode, putting us into IA-32 mode
    mov ecx, 0xC0000080 ; IA32_EFER MSR
    rdmsr
    or eax, 1 << 8      ; LM bit
    wrmsr

    ; Step 5: Enable paging
    mov eax, cr0
    or eax, 1 << 31     ; Paging enable
    mov cr0, eax

    ; Step 6: Jump to 64-bit code
    jmp 0x20:start_lm
[BITS 64]
start_lm:
    xor eax, eax ; Long mode only needs CS
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0xA000

no_lm: ; No 64-bit message
    mov eax, 0x0C4F0C4E
    mov [0xB8000], eax
    mov eax, 0x0C340C36
    mov [0xB8004], eax

    jmp $

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

times 0x1000-($-$$) nop ; 1KiB of stage2 is a lot, but can be useful
