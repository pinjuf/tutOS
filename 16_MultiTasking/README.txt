Chapter 16 - Multitasking

Of course, an OS needs to be able to be handle multiple
processes at once. In our case, we will do this using
a single threaded task switching system, meaning that
every process runs for some time, before the kernel jumps
to the next one.

FAQ:
    1) Why don't you use a linked list to keep track of processes?
        We don't need it. Yet.
