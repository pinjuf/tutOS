Chapter 17 - Ext2fs driver

The second extended filesystem was the standard filesystem for Linux
and similar *NIX kernel, before being replaced with ext3 and the current
ext4, as well as alternatives such as BTRFS. Ext2 is inode-based, meaning
that EVERYTHING in the FS can be represented as a data structure, an "inode",
that describes the file as well as the blocks where it can be found.

FAQ:
    1) What is this weird ext2_lsdir?
        For now, it is more of a demo. Later on, we should be able to implement
        an actual DIR and readdir() in the VFS.
