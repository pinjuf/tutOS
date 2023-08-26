#include "stdio.h"
#include "unistd.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: rmdir <path>\n");
        return 1;
    }

    if (rmdir(argv[1]) == -1) {
        puts("rmdir: could not remove directory\n");
        return 1;
    }
}
