ENTRY(_start);

SECTIONS {
    . = 0x400000;
    . = ALIGN(4096);
    .text : {
        *(.text)
    }

    . = ALIGN(4096);
    .data : {
        *(.data)
    }

    . = ALIGN(4096);
    .bss : {
        *(.bss)
    }

    . = ALIGN(4096);
    .rodata : {
        *(.rodata)
    }
}
