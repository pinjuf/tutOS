Chapter 19 - Virtual File System

We now have "multiple" (2) file system drivers pretty much set up:
ext2 and FAT32. But of course, it would be very difficult and annoying
to choose a certain file from a certain filesystem, forcing us to directly
call an FS driver for every open/read/write/close we would want to do.
To avoid this, we must implement a VFS, a virtual file system. This is
not a "Filesystem" in the usual sense, but rather an interface to all
mounted filesystems. Here's an example:
    - FAT32 partition (d=1, p=1) mounted on /mnt
    - open(/mnt/testdir/testfile.txt) is issued
    - VFS determines the correct mount point to be /mnt/
    - VFS asks the FAT32 driver for a file handle for testdir/testfile.txt
    - FAT32 driver creates a file handle, consisting of a standardized part
            and a driver-internal part
    - VFS gets the handle and passes it/a file descriptor to it on

FAQ:
