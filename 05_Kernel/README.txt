Chapter 05 - Kernel

Now that we have everything loaded and set up, we
are ready for the next step: the kernel! This is
they actual main component of our OS, and will
be responsible for quite literally EVERYTHING.

FAQ:
    1) Why is there .kentry section and a _kentry function?
        This simplifies making sure the linker actually puts
        our entry function at the start of everything, and
        allows for alignment optimistaions later on.
    2) Why are there so many CFLAGS?
        They're just necessary. A lot of optimisation and
        usage of some libraries can completely break a kernel,
        since the compiler expects to compile for a "normal"
        system.
    3) Why do you compile without SSE?
       SSE (a SIMD instruction extension) would need to be
       checked for and enabled to allow its use. This is not
       difficult, and other SSE versions/SIMD extensions are
       generally equally simple to set up. But in this project,
       that is simply not necessary. Also, fuck AVX-512.
