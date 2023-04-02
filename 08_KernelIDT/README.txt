Chapter 08 - Kernel IDT

You guessed it: We are setting up Long Mode Interrupts!
For this we will need a lot of stuff: An IDT and functions
to set it up, ISR assembly stubs & C routines, a PIC driver,
and all that that entails.

FAQ:
    1) What's a PIC?
    2) What's a PIT?
    3) Why's there an interrupt when I press a key, but only once?
