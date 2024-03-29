%define BPOB 0x7E00 ; "Boot Pass-On Block", stores data to be used by the kernel

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

struc bpob ; Needs to match kernel/main.h
    .vbe_info:      resb vbe_info_size      ; I know, I know, it could be cleaner and more efficient...
    .vbe_mode_info: resb vbe_mode_info_size
    .ap_count:      resb 1
    .ap_stack:      resq 1
    .ap_entry:      resq 1
endstruc
