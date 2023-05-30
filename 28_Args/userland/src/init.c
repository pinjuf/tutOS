#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char * argv[]) {
    char * cmdbuf = malloc(256);
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
            if (c == '\b' && curr > cmdbuf)
                curr--;
            else
                *curr++ = c;
            putc(c);
        }

        *curr = '\0';

        putc('\n');

        if (curr == cmdbuf)
            continue;

        if (!strcmp(cmdbuf, "exit"))
            exit(0);

        pid_t p = fork();
        if (p == 0) {
            // We need to transform cmdbuf into a char*argv[]
            char ** argv = malloc(sizeof(char*) * 16);
            int argc = 0;
            char * curr = cmdbuf;
            while (*curr) {
                argv[argc++] = curr;
                while (*curr && *curr != ' ')
                    curr++;
                if (*curr)
                    *curr++ = '\0';
            }
            argv[argc] = NULL;

            exec(cmdbuf, argv);
            puts("could not exec command\n");
            exit(0);
        }

        waitpid(p);
    }

    free(cmdbuf);

    return 0;
}
