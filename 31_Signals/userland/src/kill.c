#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char * argv[]) {
    if (argc != 2 && argc != 3) {
        puts("Usage: kill <pid> [signal]\n");
        return 1;
    }

    pid_t pid = atoi(argv[1], 10);
    int signal = SIGTERM;
    if (argc == 3) {
        signal = atoi(argv[2], 10);
    }

    int ret = kill(pid, signal);
    if (ret == -1) {
        puts("kill failed\n");
        return 1;
    }

    return 0;
}
