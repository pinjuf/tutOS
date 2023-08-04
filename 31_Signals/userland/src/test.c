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
    puts("Entering...\n");

    signal(SIGUSR1, sighandle);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    //sigprocmask(SIG_BLOCK, &mask);

    alarm(10);
    pause();

    puts("Exiting...\n");

    return 0;
}
