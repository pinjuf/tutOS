[BITS 16]
[ORG 0x7C00]

bpb: ; Boot parameter block (necessary on some BIOSes)
    jmp boot_start
    nop

    db "mkdos.fs" ; OEM name
    dw 512        ; bytes per sector
    db 1          ; sectors per cluster
    dw 1          ; reserved sectors
    db 2          ; number of FATs
    dw 224        ; root directory entries
    dw 2880       ; total sectors
    db 0xf0       ; media descriptor
    dw 9          ; sectors per FAT
    dw 18         ; sectors per track
    dw 2          ; number of heads
    dd 0          ; hidden sectors
    dd 0          ; total sectors (if above is 0)

ebr: ; Extended boot record
    db 0x00          ; drive number
    db 0x00          ; reserved
    db 0x29          ; extended boot signature
    dd 0x12345678    ; volume serial number
    db "ABCDEFGHIJK" ; volume label
    db "FAT12   "    ; file system type

boot_start:
    xor ax, ax
    mov ds, ax ; Data selector
    mov es, ax ; Extra selector
    mov ss, ax ; Stack selector
    mov sp, 0x9000 ; Stack grows DOWNWARDS on x86!

    sti

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
    mov cx, 19   ; String length
    xor dx, dx   ; Start row/line
    mov es, dx   ; String selector
    mov bp, msg  ; String offset
    int 0x10

    jmp $

msg db "Hello, OSDev world!"

times 510-($-$$) nop
dw 0xAA55
