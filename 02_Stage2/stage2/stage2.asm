[ORG 0x9000]
[BITS 16]

stage2_start:
    mov ah, 0x06 ; "Scroll up"
    mov al, 0x00 ; Clear everything
    mov bh, 0x07 ; Attributes: Light gray on black
    xor cx, cx   ; Top left corner
    mov dh, 24   ; Last row
    mov dl, 79   ; Last column
    int 0x10

    mov ah, 0x13 ; "Print string"
    mov al, 0b01 ; Update cursor
    mov bh, 0    ; Page number
    mov bl, 0x07 ; Attributes: Light gray on black
    mov cx, 30   ; String length
    xor dx, dx   ; Start row/line
    mov es, dx   ; String selector
    mov bp, msg  ; String offset
    int 0x10

    jmp $

msg db "Hello from BIOS-loaded memory!"

times 0x1000-($-$$) nop ; 1KiB of stage2 is a lot, but can be useful
