#include "libtut.h"
#include "unistd.h"

int stdout, * stdin;

void _start(int argc, char * argv[]) {
    stdout = open("/dev/tty", O_WRONLY);
    stdin = open("/dev/tty", O_RDONLY);

    int status = main(argc, argv);

    close(stdout);
    close(stdin);

    exit(status);
}

void pit_msleep(size_t ms) {
    int pit = open("/dev/pit0", O_RDONLY);
    size_t ticks;

    read(pit, &ticks, 8);

    size_t now = ticks;

    // We have PIT0_FREQ ticks per second
    while (now + PIT0_FREQ/1000 * ms > ticks) {
        read(pit, &ticks, 8);
    }

    close(pit);
}
