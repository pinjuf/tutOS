#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define CMDBUF_SZ 0x200

void sigchld(int signum) {
    sigreturn();
}

int main(int argc, char * argv[]) {
    char * cmdbuf = malloc(256);
    char c;

    signal(SIGCHLD, sigchld);

    puts("< tutOS sh >\n");

    while (true) {
        char * curr = cmdbuf;
        memset(cmdbuf, 0, CMDBUF_SZ);

        int status = 0;

        puts("$> ");

        while (1) {
            if (!read(stdin, &c, 1)) // EOF or whatever
                exit(0);

            if (c == '\n') {
                break;
            } else if (c == '\b' && (curr > cmdbuf)) {
                curr--;
                putchar(c);
            } else {
                *curr++ = c;
                putchar(c);
            }
        }

        *curr = '\0';

        putchar('\n');

        if (curr == cmdbuf)
            continue;

        if (!strcmp(cmdbuf, "exit"))
            exit(0);

        if (!strcmp(cmdbuf, "debug")) {
            asm volatile ("int $0x81");
            continue;
        }

        bool fg = true;
        // Let process run in background
        if (*(curr-1) == '&') {
            *(curr-1)  = '\0';
            fg = false;
        }

        char * cmdbuf2 = malloc(CMDBUF_SZ);
        memcpy(cmdbuf2, cmdbuf, CMDBUF_SZ);

        // We need to transform cmdbuf into a char*argv[]
        char ** argv = malloc(sizeof(char*) * 16);
        int argc = 0;
        char * acurr = cmdbuf2;
        while (*acurr) {
            argv[argc++] = acurr;
            while (*acurr && *acurr != ' ')
                acurr++;
            if (*acurr)
                *acurr++ = '\0';
        }
        argv[argc] = NULL;

        pid_t p = fork();
        if (p == 0) {
            exec(cmdbuf2, argv);
            puts("could not exec command\n");
            exit(1);
        }

        if (fg) {
            waitpid(p, &status, 0);
            putchar(status + '0');
            putchar(' ');
        } else {
            char buf[8];
            itoa(p, buf, 10);
            puts("bg: [");
            puts(buf);
            puts("]\n");
        }
    }

    free(cmdbuf);

    return 0;
}
