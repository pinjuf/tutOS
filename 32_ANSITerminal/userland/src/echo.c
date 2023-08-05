#include "stdio.h"

int main(int argc, char * argv[]) {
    for (int i = 1; i < argc; i++) {
        puts(argv[i]);
        putc(' ');
    }
    putc('\n');

    return 0;
}
