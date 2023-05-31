#include "unistd.h"
#include "stdio.h"

int main(int argc, char * argv[]) {

    stat * st = malloc(sizeof(stat));

    for (size_t i = 1; i < (size_t)argc; i++) {
        FILE * file = open(argv[i], FILE_R);

        if (!file) {
            puts("file not found\n");
            continue;
        }

        fstat(file, st);
    }

    free(st);

    return 0;
}
