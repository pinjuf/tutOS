Chapter 00 - The boot sector

PCs that use BIOS boot (unlike the more modern UEFI boot) have a relatively simple boot procedure.
First, the BIOS searches all recognized storage devices (floppies, hard drives, etc.) for one
whose first sector (512 bytes long) ends with 0xAA55. This sector is called a boot sector, and
plays an important role in OSDev, even outside of the boot procedure as it is also partially used
for partitioning and filesystems. Once such a sector is found, it is loaded to 0x7C00. The BIOS 
directly jumps to this address: This is where we get control! But be careful: At this point, we
are still operating in the 16-bit mode ("Real mode"). This restricts our use of registers, memory
ranges, and data types.

Selectors in Real mode

Selectors somehow are annoying, useful, overcomplicated, and omnipresent. For now, the only thing
that matters to us is: Selectors mean offsets. To be exact:
    Physical address = Address + Segment * 0x10
In 16-bit mode, these selectors are 16 bits wide, though the selectors themselves don't differ a
lot from what we'll come to find once we will be using 32- and 64-bit mode. Here they are:
    - CS : Code selector  | Applied at anything related to code execution
    - DS : Data selector  | Applied at anything memory access related
    - SS : Stack selector | Applied to anything stack related
    - ES : Extra selector | Used by BIOS, but can also be used for custom behaviour
    - FS : Extra selector | Used by BIOS, but can also be used for custom behaviour
    - GS : Extra selector | Used by BIOS, but can also be used for custom behaviour
In our very first boot sector, we zero out every selector (so essentially nothing happens).
But to make our assembler aware that this will be executed at 0x7C00, we use [ORG 0x7C00].
In some case, it might be more useful to set most selectors to 0x7C0.

FAQ:
    1) What's this BPB and EBR stuff?
        The Boot Parameter Block and the Extended Boot Record can give some extra information to the BIOS,
        that may sometimes be necessary. This is technically defined in the FAT-specifications.
    2) Why not use the selectors for the 0x7C00 offset?
        Because stuff will get annoying quickly if we start juggling selectors now already.
        For now, it's easier to just use the 0x7C00 assembler offset.
    3) My hexdump of our sector shows that it ends with 55 AA. Why not AA 55?
        x86 uses little-endian, so the least significant byte is stored first, and the most significant
        byte is stored last.
    4) How do these register names work?
        At their base, our registers really are just called A, B, C & D (and some other specialized ones).
        But to clarify what part of them we really want to access (the same registers will be available in
        higher modes), we use a suffix to indicated this:
            - l : Low byte   (first 8 bits)
            - h : High byte  (next 8 bits)
            - x : Everything (first 16 bits)
        Even in higher mode, we will still be able to use al, bh, cx, etc. to access exactly the same
        parts of registers.
    5) Why did you use xor ax, ax instead of mov ax, 0?
        The xor method is faster, more efficient, and doesn't obfuscate that much.
