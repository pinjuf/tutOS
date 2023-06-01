#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: ls <path>\n");
        return 1;
    }

    FILE * fd = open(argv[1], FILE_R);
    if (fd == 0) {
        puts("file not found\n");
        return 1;
    }

    stat * st = malloc(sizeof(stat));
    fstat(fd, st);

    if (st->st_mode != FILE_DIR) {
        puts("ls: not a dir\n");
        return 1;
    }

    free(st);

    while (1) {
        dirent * d = readdir(fd);
        if (!d) break;
        puts(d->d_name);
        putc(' ');
        free(d);
    }
    putc('\n');

    close(fd);
}
