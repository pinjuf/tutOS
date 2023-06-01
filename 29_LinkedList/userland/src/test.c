#include "stdlib.h"
#include "stdio.h"

int main(int argc, char * argv[]) {
    puts("My arguments:\n");
    for (int i = 0; i < argc; i++) {
        puts(" - ");
        puts(argv[i]);
        putc('\n');
    }
    return 0;
}
