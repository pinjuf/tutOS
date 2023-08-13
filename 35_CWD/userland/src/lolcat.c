#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "types.h"

int main(int argc, char * argv[]) {
    stat * st = malloc(sizeof(stat));

    char colstring[] = "\033[9#m";
    uint8_t counter = 0;

    if (argc == 1) {
        argc = 2;
        argv[1] = "-"; // Last argv is always NULL, so we can afford this
    }

    for (size_t i = 1; i < (size_t)argc; i++) {
        int file;
        if (strcmp("-", argv[i]))
            file = open(argv[i], O_RDONLY);
        else
            file = stdin;

        if (file < 0) {
            puts("file not found\n");
            return 1;
        }

        fstat(file, st);

        switch(st->st_mode) {
            case FILE_DIR:
                puts("lolcat: ");
                if (file != stdin)
                    puts(argv[i]);
                else
                    puts("-");
                puts(": Is a directory\n");
                break;
            case FILE_PIPE:
            case FILE_REG:
            case FILE_DEV:
            case FILE_BLK: {
                char c;
                while (read(file, &c, 1) == 1) {
                    counter += 1;
                    counter %= 8;

                    colstring[3] = '0' + counter;

                    write(stdout, colstring, 5);
                    write(stdout, &c, 1);
                }
                break;
            }
            default:
                puts("lolcat: ");
                if (file != stdin)
                    puts(argv[i]);
                else
                    puts("-");
                puts(": Is not a file\n");
                break;
        }

        if (file != stdin)
            close(file);
    }

    free(st);

    puts("\033[0m");

    return 0;
}
