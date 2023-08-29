[ORG 0xA000]
[BITS 16]

%define MAGIC_ADDR  0x9C00
%define MAGIC_VALUE 0x6969

cli

mov ax, MAGIC_VALUE
mov word [MAGIC_ADDR], ax

jmp $

times 0x1000-($-$$) nop ; 1KiB of AP startup code is a lot, but can be useful
