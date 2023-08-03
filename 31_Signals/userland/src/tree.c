#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

void tree_dir(FILE * dir, char * current_path, size_t depth) {
    dirent * d = readdir(dir);

    while (d) {
        if (!strcmp(d->d_name, ".") \
         || !strcmp(d->d_name, "..")) {
            d = readdir(dir);
            continue;
        }

        for (size_t i = 0; i < depth; i++) {
            puts("| ");
        }

        if (depth)
            putc(' ');

        puts("|- ");
        puts(d->d_name);
        putc('\n');

        if (d->d_type == FILE_DIR) {
            char buf[1024] = {0};
            strcpy(buf, current_path);
            buf[strlen(buf)] = '/';
            strcpy(buf + strlen(buf), d->d_name);
            buf[strlen(buf)] = '/';

            FILE * fd = open(buf, O_RDONLY);
            tree_dir(fd, buf, depth + 1);
            close(fd);
        }

        d = readdir(dir);
    }
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: tree <path>\n");
        return 1;
    }

    FILE * fd = open(argv[1], O_RDONLY);
    if (fd == 0) {
        puts("file not found\n");
        return 1;
    }

    tree_dir(fd, argv[1], 0);

    close(fd);

    return 0;
}
