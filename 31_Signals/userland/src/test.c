#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

void testhandler(int sig) {
    puts(" | SIGNAL | ");
}

int main(int argc, char * argv[]) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = testhandler;
    sigaction(69, &sa);

    return 0;
}
