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

    ; Write something to the VGA buffer
    mov eax, 0x0F4B0F4F ; "OK", white on black
    mov [0xB8000], eax

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
gdt_end:

times 0x1000-($-$$) nop ; 1KiB of stage2 is a lot, but can be useful
