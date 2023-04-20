Chapter 14 - First Usermode

For now, everything has been running in kernel mode, aka "Ring 0".
Ring 0 code is essentially the ultimate authority, being able to
run pretty much any instruction the CPU supports. But most programs
shouldn't be to execute such powerful instructions, and should
instead have to go through the kernel to do so. For this, we need
our code to run in "Ring 3", which is reflected in CS.

FAQ:
