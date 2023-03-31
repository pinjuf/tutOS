Chapter 03 - Protected Mode

Of course, staying in Real Mode has some cons, in fact,
it has a lot of cons. Therefore, we must venture onwards,
into the 32-bit mode (Protected Mode)! There, we will
have access to 32-bit registers/datatypes, a lot more
CPU features, etc.! To do this, we set up a GDT, the main
structure responsible for selectors in Protected Mode.
This can be a bit annoying, but necessary.

FAQ:
    1) What is this VGA stuff?
        The BIOS has graciously set up VGA Color Text Mode!
        This means that there's an memory range, starting at
        0xB8000. Every 2 bytes are mapped to one character
        on the screen, the first byte is the actual character,
        the second one is the attributes (color, etc.).
    2) What's this CR0 register?
        x86 uses so-called Control Registers (abbr. CR),
        all indexed. They are used to specify the CPU state,
        altough other registers outside of the CR series
        may be used for that (see MSRs).
