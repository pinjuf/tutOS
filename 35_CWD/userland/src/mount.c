#include "unistd.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char * argv[]) {
    if (argc != 4) {
        puts("Usage: mount <source>/- <target> <filesystemtype>\n");
        return 1;
    }

    if (!strcmp(argv[1], "-")) {
        argv[1] = NULL;
    }

    if (mount(argv[1], argv[2], argv[3], 0, NULL) == -1) {
        puts("mount failed\n");
        return 1;
    }

    return 0;
}
