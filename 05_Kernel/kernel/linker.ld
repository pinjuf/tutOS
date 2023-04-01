ENTRY(_kentry)
OUTPUT_FORMAT("binary")
SECTIONS {
    .kentry : {
        *(.kentry)
    } =0xA000

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
