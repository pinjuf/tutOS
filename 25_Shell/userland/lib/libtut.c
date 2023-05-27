#include "libtut.h"
#include "unistd.h"

FILE * stdout, * stdin;

void _start() {
    stdout = open("/dev/tty", FILE_W);
    stdin = open("/dev/tty", FILE_R);

    main();

    close(stdout);
    close(stdin);

    while (1);
}
