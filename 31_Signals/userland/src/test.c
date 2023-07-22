#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

void sighandle(int signum) {
    if (signum == SIGSTOP) {
        puts("I was stopped!\n");
    } else if (signum == SIGCONT) {
        puts("I was continued!\n");
    } else {
        char buf[16];
        puts("Got signal #");
        itoa(signum, buf, 10);
        puts(buf);
        putc('\n');
    }

    sigreturn();
}

int main(int argc, char * argv[]) {

    signal(SIGSTOP, sighandle);
    signal(SIGCONT, sighandle);
    signal(SIGUSR1, sighandle);
    signal(SIGUSR2, sighandle);

    puts("Hello from test program!\n");

    pit_msleep(5000);
    //asm ("cli"); // Causes a #GP
    pit_msleep(5000);

    puts("Bye from test program!\n");

    return 0;
}
