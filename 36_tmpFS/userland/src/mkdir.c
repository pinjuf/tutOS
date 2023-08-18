#include "unistd.h"

int main(int argc, char * argv[]) {
    if (argc != 2) {
        puts("Usage: mkdir <dir>\n");
        return 1;
    }
    if (mkdir(argv[1]) < 0) {
        puts("mkdir: could not create dir\n");
        return 1;
    }
    return 0;
}
