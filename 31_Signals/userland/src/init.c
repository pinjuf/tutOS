#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

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
        memset(cmdbuf, 0, 256);

        int status = 0;

        puts("$> ");

        while (1) {
            read(stdin, &c, 1);

            if (c == '\n') {
                break;
            } else if (c == '\b' && (curr > cmdbuf)) {
                curr--;
                putc(c);
            } else {
                *curr++ = c;
                putc(c);
            }
        }

        *curr = '\0';

        putc('\n');

        if (curr == cmdbuf)
            continue;

        if (!strcmp(cmdbuf, "exit"))
            exit(0);

        if (!strcmp(cmdbuf, "debug")) {
            asm volatile ("int $0x81");
            continue;
        }

        if (!strcmp(cmdbuf, "spktst")) {
            FILE * pcspk = open("/dev/pcspk", O_WRONLY);

            uint32_t freq_a = 440;
            uint32_t freq_b = 660;
            uint32_t silent = 0;

            for (size_t i = 0; i < 4; i++) {
                write(pcspk, &freq_a, 4);
                pit_msleep(500);
                write(pcspk, &freq_b, 4);
                pit_msleep(500);
            }

            write(pcspk, &silent, 4);

            continue;
        }

        bool fg = true;

        // Let process run in background
        if (*(curr-1) == '&') {
            *(curr-1)  = '\0';
            fg = false;
        }

        char * cmdbuf2 = malloc(256);
        memcpy(cmdbuf2, cmdbuf, 256);

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
            waitpid(p, &status);
            putc(status + '0');
            putc(' ');
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
