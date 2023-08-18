#include "stdio.h"
#include "unistd.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: rm <file>\n");
        return 1;
    }
    if (unlink(argv[1]) < 0) {
        puts("rm: cannot remove file\n");
        return 1;
    }
    return 0;
}
