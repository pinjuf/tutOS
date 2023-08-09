Chapter 34 - Pipes

An important part of UNIX and *NIX-systems is the implementation of FIFO
devices. FIFO devices with a fixed input and output side (accessible via
a file descriptor and normal read/write commands) are known as pipes, and
can be used by processes to communicate with eachother. For example, as
you have surely tried out in your shell, you can redirect the output of
a program to be the input of another program, using the "|" (pipe) symbol.
Let's implement it!

FAQ:
