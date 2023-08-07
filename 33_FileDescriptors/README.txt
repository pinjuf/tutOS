Chapter 33 - File Descriptors

Up to now, when a program requests a file, it got
a pointer to a filehandle_t structure in return.
As you might have perhaps noticed, Linux doesn't
do it like that. Instead, Linux gives the program
a number, a file descriptor, that can then be used
to in syscalls. This has multiple advantages:
Special numbers can be hard-coded for special
purposes, like stdin, stdout and stderr. Furthermore,
it allows for anonymous data structure, allowing
programs to read and write to objects not strictly
represented by a file (technically on Linux, it still is),
like pipes!

FAQ:
