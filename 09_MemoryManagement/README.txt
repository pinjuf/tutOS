Chapter 09 - Memory Management

If you've ever programmed in a non-baby language,
you know malloc() and free() (hopefully). Now it
is time to implement something similar on our OS.
It needs to give memory from our new heap memory
to whoever wants it, and free it when it is ordered
to. We will have to implement a paging system with
memory mapping, as well as heap memory and heap
memory management. There are many different algo-
rithms for dynamic memory allocation, like a buddy
system. In our case though, we will just use a bitmap
to represent our memory.

FAQ:
    1) Why aren't there two separate heaps, one for the kernel, and one for the userland to come?
        It won't make that much of a difference, and I'm too lazy.
