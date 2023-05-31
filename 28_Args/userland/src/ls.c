#include "unistd.h"
#include "stdio.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: ls <path>\n");
        return 1;
    }

    FILE * fd = open(argv[1], FILE_R);
    if (fd == 0) {
        puts("File not found\n");
        return 1;
    }

    while (1) {
        dirent * d = readdir(fd);
        if (!d) break;
        puts(d->d_name);
        putc(' ');
    }
    putc('\n');

    close(fd);
}
