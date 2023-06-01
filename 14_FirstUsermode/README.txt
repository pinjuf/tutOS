Chapter 14 - First Usermode

For now, everything has been running in kernel mode, aka "Ring 0".
Ring 0 code is essentially the ultimate authority, being able to
run pretty much any instruction the CPU supports. But most programs
shouldn't be to execute such powerful instructions, and should
instead have to go through the kernel to do so. For this, we need
our code to run in "Ring 3", which is reflected in CS.

FAQ:
    1) Why do you check for the U/S flag while page mapping?
        Because of the U/S hierarchy direction, all HIGHER LEVEL
        table entries must have U/S set in order for LOWER LEVEL
        table entries to actually have a valid U/S flag.
    2) Why is there a #GP after some time?
        That is wanted (see usermode_code()), and shows that
        Ring-3 code cannot use some instructions (cli in this case).
