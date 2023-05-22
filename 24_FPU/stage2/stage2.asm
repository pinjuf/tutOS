[ORG 0x9000]
[BITS 16]

%define BPOB 0x7E00 ; "Boot Pass-On Block", stores data to be used by the kernel

%define VESA_WIDTH  1024
%define VESA_HEIGHT 768
%define VESA_BPP    32

struc vbe_info
    .signature:    resb 4
    .version:      resb 2
    .oem:          resd 1
    .capabilities: resd 1
    .video_modes:  resd 1
    .video_memory: resw 1
    .rev:          resw 1
    .vendor:       resd 1
    .product_name: resd 1
    .product_rev:  resd 1
    .reserved:     resb 222
    .oem_data:     resb 256
endstruc

struc vbe_mode_info
    .attributes:   resw 1
    .window_a:     resb 1
    .window_b:     resb 1
    .granularity:  resw 1
    .window_size:  resw 1
    .segment_a:    resw 1
    .segment_b:    resw 1
    .win_func_ptr: resd 1
    .pitch:        resw 1

    .width:        resw 1
    .height:       resw 1
    .w_char:       resb 1
    .y_char:       resb 1
    .planes:       resb 1
    .bpp:          resb 1
    .banks:        resb 1
    .memory_model: resb 1
    .bank_size:    resb 1
    .image_pages:  resb 1
    .reserved0:    resb 1

    .red_mask:                resb 1 ; Note: We assume a pattern like 0x00RRGGBB
    .red_position:            resb 1
    .green_mask:              resb 1
    .green_position:          resb 1
    .blue_mask:               resb 1
    .blue_position:           resb 1
    .reserved_mask:           resb 1
    .reserved_position:       resb 1
    .direct_color_attributes: resb 1

    .framebuffer:         resd 1
    .off_screen_mem_off:  resd 1
    .off_screen_mem_size: resw 1
    .reserved1:           resb 206
endstruc

struc bpob
    .vbe_info:      resb vbe_info_size      ; I know, I know, it could be cleaner and more efficient...
    .vbe_mode_info: resb vbe_mode_info_size
endstruc

stage2_start:
    cli

    ; Get the VBE Info Structure
    mov byte [BPOB + bpob.vbe_info + vbe_info.signature + 0], 'V'
    mov byte [BPOB + bpob.vbe_info + vbe_info.signature + 1], 'B'
    mov byte [BPOB + bpob.vbe_info + vbe_info.signature + 2], 'E'
    mov byte [BPOB + bpob.vbe_info + vbe_info.signature + 3], '2'

    xor ax, ax
    mov es, ax
    mov ax, 0x4F00 ; GET VESA INFO
    mov di, BPOB + bpob.vbe_info
    int 0x10

    cmp ax, 0x004F
    jne no_vesa

find_vbe_mode:
    ; Get the current mode index
    mov si, [BPOB + bpob.vbe_info + vbe_info.video_modes]
    mov bx, [BPOB + bpob.vbe_info + vbe_info.video_modes + 2]
    mov fs, bx
    add si, [vbe_modecount]
    add word [vbe_modecount], 2
    mov ax, [fs:si]

    cmp ax, 0xFFFF
    je no_vesa

    xor cx, cx
    mov es, cx ; VBE mode info structure selector
    mov cx, ax ; Mode index

    push cx    ; Keep it for later!

    mov di, BPOB + bpob.vbe_mode_info
    mov ax, 0x4F01 ; GET VESA MODE INFO
    int 0x10

    pop cx

    cmp ax, 0x004F
    jne no_vesa

    ; Check if it is the correct mode
    mov ax, [BPOB + bpob.vbe_mode_info + vbe_mode_info.width]
    cmp ax, VESA_WIDTH
    jne find_vbe_mode

    mov ax, [BPOB + bpob.vbe_mode_info + vbe_mode_info.height]
    cmp ax, VESA_HEIGHT
    jne find_vbe_mode

    mov al, [BPOB + bpob.vbe_mode_info + vbe_mode_info.bpp]
    cmp al, VESA_BPP
    jne find_vbe_mode

    ; Actually select the mode
    mov ax, 0x4F02 ; SET VESA MODE
    mov bx, cx
    ; Because we are not using the CRTCInfo struct/setting bit 11, we do not need to pass anything in ES:DI
    int 0x10

    cmp ax, 0x004F
    jne no_vesa

    ; Prepare for Protected Mode
    lgdt [gdtr] ; Load the GDT
    
    mov eax, cr0
    or al, 1     ; Set the protection enable bit
    mov cr0, eax

    jmp 0x08:reload_cs ; CS needs to be set this way

no_vesa:
    mov ax, 0x1301
    mov bx, 0x000C
    mov cx, 5      ; String length
    xor dx, dx
    mov es, dx
    mov bp, vbe_error
    int 0x10
    jmp $

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

    ; Enabling MMX, SSE & SSE2
    ; Step 1: Check for features
    mov eax, 0x1
    cpuid
    test edx, 1 << 25 ; SSE CPUID
    jz no_sse
    test edx, 1 << 26 ; SSE2 CPUID
    jz no_sse

    ; Step 2: Enable SSE etc.
    mov eax, cr0
    and ax, ~(1 << 2) ; Disable CR0.EM
    or  ax,  1 << 1   ; Enable  CR0.MP
    mov cr0, eax

    mov eax, cr4
    or ax, 0b11 << 9 ; Enable CR4.OSFXSR and CR4.OSXMMEXCPT
    mov cr4, eax

    ; Getting to long mode:
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

no_lm: ; No 64-bit message (only for VGA)
    mov eax, 0x0C4F0C4E
    mov [0xB8000], eax
    mov eax, 0x0C3F0C45
    mov [0xB8004], eax

    jmp $

no_sse: ; SSE? message (only for VGA)
    mov eax, 0x0C530C53
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

vbe_modecount: dw 0
vbe_error: db "VESA?"

times 0x1000-($-$$) nop ; 1KiB of stage2 is a lot, but can be useful
