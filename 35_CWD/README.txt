Chapter 35 - Current working directory

On *NIX, every process has a "Current Working Directory" (a.k.a. cwd/pwd), which is the directory
that the process is currently "in". When a process starts, its cwd is inherited from its parent,
and any relative paths used by the process in syscalls is relative to the CWD.

FAQ:
