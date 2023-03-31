Chapter 04 - Long mode

You can write a complete OS in Protected Mode. In fact, that was
pretty much the only way to do it, before 64-bit architectures
came along. In our case, this 64-bit mode is called Long Mode.
Setting it up, however, is a bit more difficult, as it requires
paging, PAE, and some other bits set. But for a bit of work, we
will have access to 64-bit registers, and also 64-bit memory
(well, technically, 48 virtual and 42 physical bits in most cases).
Now, when you run this, you will see a lovely little OK64 message
at the top left corner of the screen, indicating that our jump into
Long Mode has worked, allowing us to use 64-bit registers!

FAQ:
    1) What's paging?

    2) Where's the Page Table?
        We use Page Size Extension (though not always necessary in
        LM) and the Size Bit on the PDT level, allowing for an
        id-mapped 4 MiB page.

    3) What's this IA32-mode?
        When AMD64/x86_64 was introduced, it was necessary for it
        to be compatible with older x86 (32-bit) programs. This
        is still noticeable today, as you can run 32-bit programs
        on 64-bit systems. When this happens, these programs run
        in IA32 Compatibility Mode. In our case, we have to pass
        through this mode while travelling from 32-bit to 64-bit.
