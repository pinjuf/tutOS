#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

int main(int argc, char * argv[]) {

    stat * st = malloc(sizeof(stat));

    for (size_t i = 1; i < (size_t)argc; i++) {
        FILE * file = open(argv[i], FILE_R);

        if (!file) {
            puts("file not found\n");
            continue;
        }

        fstat(file, st);

        switch(st->st_mode) {
            case FILE_REG: {
                char * buf = malloc(st->st_size);
                read(file, buf, st->st_size);
                puts(buf);
                free(buf);
                break;
            }
            case FILE_DIR:
                puts("cat: ");
                puts(argv[i]);
                puts(": Is a directory\n");
                break;
            case FILE_BLK: {
                char c[512]; // Reading in sectors is faster
                while (1) {
                    int r = read(file, c, 512);
                    if (r == 0) break;
                    write(stdout, c, r);
                }
                break;
            }
            case FILE_DEV: {
                char c;
                while (read(file, &c, 1))
                    putc(c);
                break;
            }
            default:
                puts("cat: ");
                puts(argv[i]);
                puts(": Is not a file\n");
                break;
        }

        close(file);
    }

    free(st);

    return 0;
}
