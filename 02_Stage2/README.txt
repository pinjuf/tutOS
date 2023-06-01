Chapter 02 - Stage 2

As you might have noticed, the BIOS only loads the first 512 bytes
(the boot sector) from our image. But of course, our OS will be
much larger than that. Therefore, we need to load the rest of the
image. Luckily there's a BIOS interrupt just for that, which we
use to load the rest, and then just jump to it. We have chosen to
put it at 0x9000, directly after our stack. For now, we'll just
show a little message.

FAQ:
    1) This other tutorial doesn't to it that way!
        Many tutorials use Cylinder-Head-Sector (CHS) addressing
        for image loading. This is an old method, that necessitates
        uncomplicated, yet annoying calculations for the CHS values.
        By using a Disk Address Packet Structure, we can use modern
        Linear Block Addressing (LBA), though this may not be
        available for older and smaller storage types.

    2) Why do you use cat to combine the boot sector & stage2?
        In the end, our image isn't really that complicated.
        The first 512 bytes are the boot sector, the rest
        is, well, the rest. cat does the job perfectly well.

    3) Why now go into Protected Mode while still in the boot sector?
        Further down the line, we will use some BIOS interrupts
        to set rather complicated to do by yourself stuff up.
        For this, we need some space that the BS simply doesn't
        offer.
