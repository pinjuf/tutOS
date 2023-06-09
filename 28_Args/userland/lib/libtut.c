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
