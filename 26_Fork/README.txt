Chapter 26 - fork()

The fork() syscall is an important call that
essentially duplicates a process, with one
being the parent, and the other being the child.
This by itself is not that useful, but for example,
the child can then call exec() to transform itself
into a new process.

FAQ:
