Chapter 06 - VGA Driver

You have already seen us use the VGA buffer to
write and show text on the screen. Now that we
have a kernel, it is time to write a driver,
that will do the annoying stuff for us, and
give us a simple interface to everything.

FAQ:
    1) Why do you use stddef.h and stdint.h?
