Chapter 07 - Kernel GDT

Altough the GDT plays a far lesser role in Long Mode
than it does in Protected Mode, it still needs to be
set up, as multiple aspects of our kernel will
depend on being able to easily control it.

FAQ:
