Chapter 31 - Signals

If this world were logical, signals would be like interrupts for userland programs.
This world is not logical. Still however, signals bear a resemblance to IRQs.
When a process receives a signal, the kernel determines the appropriate course of
action. Here the 3 standard POSIX responses:

 - SIG_DFL ("default"): The kernel handles the interrupt in its default manner.
        (e.g. ignore, kill process, etc.)
 - SIG_IGN ("ignore"):  The signal is ignored.
 - function pointer   : The function pointed to is executed by the process itself.

How these signals are to be handles can be set by the program using a sigaction
syscall, that passes along a structure describing the handling.

SIG_DFL and SIG_IGN are far from any challenge to implement, letting the process
choose a function to execute can however present some difficulties. For example,
we cannot simple set RIP and pass some args (how would the function return).
Luckily, as we try to stick to generic conventions and the SysV-ABI, we can risk
putting the return address onto the stack (anything below RSP is safe to write
to if the program knows its standards) However, just the be sure, a process
will have a flag indicating whether it is currently handling a signal. If this
is the case, its registers will be saved to a separate register structure.
Using another syscall, the process will be able to reset that flag and return
to its original execution.

FAQ:
    - i don't get it either, but i think it's working...

man-references:
    - sigaction
    - sigreturn
    - sigaltstack
    - sigprocmask
