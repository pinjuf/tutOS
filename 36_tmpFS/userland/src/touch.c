#include "unistd.h"
#include "stdio.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: touch <file>\n");
        return 1;
    }
    if (creat(argv[1]) < 0) {
        puts("touch: could not create file\n");
        return 1;
    }
}
