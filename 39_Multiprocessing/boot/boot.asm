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
    mov sp, 0x9000 ; Stack grows DOWNWARDS on x86, so we can put the stage2 there

    mov [bootdrive], dl

    ; Setup the A20 line
    in al, 0x92
    or al, 0b10
    out 0x92, al

    ; Load first 64K
    mov dl, [bootdrive]
    mov ah, 0x42
    mov si, daps
    int 0x13

    ; Load next 64K
    mov byte  [daps_sect], 0x80
    mov word  [daps_offs], 0
    mov word  [daps_sel],  0x1900
    mov dword [daps_lbal], 0x81
    mov dword [daps_lbah], 0x00
    mov dl, [bootdrive]
    mov ah, 0x42
    mov si, daps
    int 0x13

    jmp 0x9000

bootdrive db 0

daps: ; Disk Address Packet Structure
    daps_size:  db 0x10   ; Size of this structure
    daps_res:   db 0x00   ; Reserved
    daps_sect:  dw 0x80   ; Number of sectors to read (max value for QEMU) (int 0x13 sets this to the amount of sectors actually read)
    daps_offs:  dw 0      ; Offset in buffer (see daps_sel)
    daps_sel:   dw 0x900  ; Segment selector for buffer (= write to 0x9000)
    daps_lbal:  dd 0x1    ; Low 32 bits of LBA
    daps_lbah:  dd 0x0    ; High 32 bits of LBA

times 510-($-$$) nop ; Pad for 512 byte boot sector
dw 0xAA55            ; Boot signature
