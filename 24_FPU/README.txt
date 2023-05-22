Chapter 24 - Floating Point Unit

Floating point operations are a bit weird on x86.
They usually require a so-called Floating Point Unit
to actually do quick hardware-level math with them.
Originally, there was a dedicated x87 FPU chip, but
by now, they are generally integrated into the CPU
and have been replaced by more modern MMX, SSE & SSE2.
However, this features must be activated, and their
internal registers must be stored during task switching.

FAQ:
