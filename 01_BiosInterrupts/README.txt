Chapter 01 - BIOS Interrupts

A universal concept among modern computers is that of interrupts.
When an interrupt is received, the CPU looks up what to do, and
executes the appropriate code. Interrupts can be triggered from
outside sources (e.g. an extern chipset, other peripherals) or
from intern sources, like a program using the int-instruction
or something causing an exception. Interrupts generally have an
associated index number, denoting its function. Interrupts can
be disabled using cli (CLear Interrupt) and enabled using sti
(SeT Interrupt), though some exceptions will still force you
to do something (or triple-fault you). While in Real Mode, the
BIOS has graciously set up some interrupts for us. These BIOS
interrupts are often powerful implementations of complex
behaviour, so some programmers tend to just stay in Real Mode
as to be able to use the BIOS that way, forfeiting more powerful
CPU modes. In our case, we use an interrupt to print something
to the BIOS screen. Arguments and return values are passed via
the registers.

FAQ:
    1) My tutorial did that differently!
        We use the more modern interrupt 0x10:AH=0x13. It may not
        be available everywhere, but if you PC is younger than
        Linux, it's a safe bet. Some tutorials instead opt to write
        each character one by one with 0x10:AH=0x09 ("display char")
