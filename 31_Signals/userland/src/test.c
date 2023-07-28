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

    alarm(1);

    while (1);

    return 0;
}
