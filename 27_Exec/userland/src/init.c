#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main() {
    char * cmdbuf = malloc(256);
    char * temp_cmdbuf = malloc(256);
    char c;

    puts("< tutOS sh >\n");

    while (true) {
        char * curr = cmdbuf;
        memset(cmdbuf, 0, 256);

        puts("$> ");

        while (1) {
            read(stdin, &c, 1);
            if (c == '\n')
                break;
            if (c == '\b')
                curr--;
            else
                *curr++ = c;
            putc(c);
        }

        *curr = '\0';

        putc('\n');

        if (curr == cmdbuf)
            continue;

        // The parent will pretty much immediately clear cmdbuf
        strcpy(temp_cmdbuf, cmdbuf);
        pid_t p = fork();
        if (p == 0) {
            exec(temp_cmdbuf);
            puts("could not exec command");
            exit(0);
        }

        waitpid(p);
    }

    free(cmdbuf);

    return 0;
}
