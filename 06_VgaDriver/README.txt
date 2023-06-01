Chapter 06 - VGA Driver

You have already seen us use the VGA buffer to
write and show text on the screen. Now that we
have a kernel, it is time to write a driver,
that will do the annoying stuff for us, and
give us a simple interface to everything.

FAQ:
    1) Why do you use stddef.h and stdint.h?
        These parts of the standard lib are not dependent
        on anything else, and supply a lot of useful
        (type) definitions.
    2) What is this qemu_puts stuff?
        QEMU offers us a debug port on 0xE9, where we
        can just write whatever. This can become useful
        once we start doing more with our screen.
    3) Why do you use kputs?
        It's better to have one interface to all other
        text interfaces, especially once we add more of
        them.
