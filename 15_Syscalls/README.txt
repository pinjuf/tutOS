Chapter 15 - System Calls

Altough most code in an OS runs in Usermode, it still
needs to be able to do stuff that would require higher
(Ring-0) permissions, like writing to VGA memory for
example, or using port I/O. For, OSs use system calls -
instructions (generally interrupts/specialized instructions)
that user code can execute, triggering syscall handling code
running in the kernel. The kernel can then check the validity
of the request, execute it, and return some data. For us, it
is now time to integrate this.

FAQ:
