#include "libtut.h"
#include "unistd.h"

void _start(int argc, char * argv[]) {
    int status = main(argc, argv);

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
