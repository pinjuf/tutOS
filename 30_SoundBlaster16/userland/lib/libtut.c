#include "libtut.h"
#include "unistd.h"

FILE * stdout, * stdin;

void _start(int argc, char * argv[]) {
    stdout = open("/dev/tty", FILE_W);
    stdin = open("/dev/tty", FILE_R);

    int status = main(argc, argv);

    close(stdout);
    close(stdin);

    exit(status);
}

void pit_msleep(size_t ms) {
    FILE * pit = open("/dev/pit0", FILE_R);
    size_t ticks;

    read(pit, &ticks, 8);

    size_t now = ticks;

    // We have PIT0_FREQ ticks per second
    while (now + PIT0_FREQ/1000 * ms > ticks) {
        read(pit, &ticks, 8);
    }

    close(pit);
}
