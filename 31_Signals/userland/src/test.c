#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

void sighandle(int signum) {
    if (signum == SIGSTOP) {
        puts("I was stopped!\n");
    } else if (signum == SIGCONT) {
        puts("I was continued!\n");
    } else if (signum == SIGALRM) {
        puts("Got alarm!\n");
        alarm(1);
    } else if (signum == SIGUSR1) {
        puts("Got SIGUSR1!\n");
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
    signal(SIGALRM, sighandle);
    signal(SIGUSR1, sighandle);

    raise(SIGUSR1);
    puts("Raised SIGUSR1!\n");

    return 0;
}
