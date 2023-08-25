ENTRY(_kentry)
OUTPUT_FORMAT("binary")
SECTIONS {
    . = 0xA000;

    .kentry : AT(0xA000) {
        *(.kentry)
    }

    .text : {
        *(.text)
    }

   .data : {
       *(.data)
   }

   .bss : {
       *(.bss)
       *(COMMON)
   }
   .rodata : {
       *(.rodata)
   }

   /DISCARD/ : {
       *(.comment)
       *(.note)
       *(.debug)
   }
}
