Chapter 38 - Advanced Programmable Interrupt Controller (APIC)

The old 8259 PIC is, well, old. Because of this, it has been replaced by the modern
APIC. The APIC is a much more advanced PIC, and is used in all modern computers. It
is programmed via memory-mapped I/O, and has many more features than the 8259 PIC.
Each processor has its own APIC. Furthermore, there is a number (generally 1) of
I/O APICs, which are used to route interrupts from I/O devices to the processors.
APICs, altough more modern, have already been replaced by xAPICs and x2APICs, but
that is beyond the scope of this OS. Let's replace the 8259 PIC with the APIC!

FAQ:
