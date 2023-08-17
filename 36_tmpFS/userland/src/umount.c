#include "stdio.h"
#include "unistd.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        puts("Usage: umount <target>\n");
        return -1;
    }

    if (umount(argv[1]) < 0) {
        puts("umount failed\n");
        return -1;
    }

    return 0;
}
