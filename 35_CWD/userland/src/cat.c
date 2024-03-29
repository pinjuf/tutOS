#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"
#include "string.h"

int main(int argc, char * argv[]) {
    stat * st = malloc(sizeof(stat));

    int adjusted_argc = (argc - 1) ? argc : 2; // cat without args reads from stdin

    for (size_t i = 1; i < (size_t)adjusted_argc; i++) {
        int file;
        if (argc > 1) {
            if (strcmp("-", argv[i]))
                file = open(argv[i], O_RDONLY);
            else
                file = stdin;
        }
        else
            file = stdin;

        if (file < 0) {
            puts("file not found\n");
            return 1;
        }

        fstat(file, st);

        switch(st->st_mode) {
            case FILE_REG: {
                char * buf = malloc(st->st_size);
                read(file, buf, st->st_size);
                write(stdout, buf, st->st_size);
                free(buf);
                break;
            }
            case FILE_DIR:
                puts("cat: ");
                if (file != stdin)
                    puts(argv[i]);
                else
                    puts("-");
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
            case FILE_PIPE:
            case FILE_DEV: {
                char c;
                while (read(file, &c, 1))
                    putchar(c);
                break;
            }
            default:
                puts("cat: ");
                if (file != stdin)
                    puts(argv[i]);
                else
                    puts("-");
                puts(": Is not a file\n");
                break;
        }

        close(file);
    }

    free(st);

    return 0;
}
