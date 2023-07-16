#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

void sigterm(int signum) {
    puts(" nope ");
    sigreturn();
}

int main(int argc, char * argv[]) {

    signal(SIGTERM, sigterm);

    puts("Hello from test program!\n");

    pit_msleep(20000);

    puts("Bye from test program!\n");

    return 0;
}
