Chapter 08 - Kernel IDT

You guessed it: We are setting up Long Mode Interrupts!
For this we will need a lot of stuff: An IDT and functions
to set it up, ISR assembly stubs & C routines, a PIC driver,
and all that that entails.

FAQ:
    1) What's a PIC?
        The Programmable Interrupt Controller is a chip/system that
        manages hardware interrupts. It controls when they are
        triggered, if they are triggered, how they are triggered,
        and where they are triggered. On modern chipsets, there are
        2 PIC chips, a master and a slave, that must be configured
        to be aware of their identities and communicate over the 
        IRQ2 line. (the slave has a hardwired pin telling it it's
        a slave, but is doesn't know more)
    2) What's a PIT?
        The Programmable Interrupt Timer is essentially a clock that
        periodically causes interrupts. There are 3 channels, 0, 1, & 2,
        with 0 being wired to IRQ0 and 2 wired to the PC speaker.
    3) Why's there an interrupt when I press a key, but only once?
        The PS/2 controller causes IRQ1 on keypresses, then expects a
        to be read from via CPU ports. Because this doesn't happen, it
        doesn't send another interrupt.
