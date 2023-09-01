#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define CMDBUF_SZ 0x400

void sigchld(int signum) {
    sigreturn();
}

int main(int argc, char * argv[]) {
    signal(SIGCHLD, sigchld);

    const char * o_cmd = malloc(CMDBUF_SZ);

    char cwd[0x100];

    // Shell capable of PIPES and REDIRECTS
    puts("< tutOS init shell >\n");

    while (1) {
        char * cmd = (char*)o_cmd;
        char * cur = cmd;
        memset(cmd, 0, CMDBUF_SZ);

        getcwd(cwd, sizeof(cwd));

        // TODO: printf for userland
        puts(cwd);
        puts("$> ");

        while (1) {
            char c;
            if (read(0, &c, 1) != 1) {
                exit(0);
            }

            if (c == '\n') {
                break;
            } else if (c == '\b') {
                if (cur != cmd) {
                    cur--;
                    putchar('\b');
                }
            } else {
                *cur++ = c;
                putchar(c);
            }
        }

        putchar('\n');

        if (cur == cmd) {
            continue;
        }

        // Pre-strip lead/trail-ing spaces
        while (*cmd == ' ') cmd++;
        char * end = cmd + strlen(cmd) - 1;
        while (*end == ' ') *end-- = 0;

        // Shell built-ins
        if (!strcmp(cmd, "exit")) {
            exit(0);
        } else if (!strncmp(cmd, "cd ", 3)) {
            if (chdir(cmd + 3) < 0)
                puts("cd: no such file or directory\n");
            continue;
        }

        // 1) How many commands? Divide by pipes and NULL terminate
        int ncmds = 1;
        for (char * c = cmd; *c != 0; c++) {
            if (*c == '|') {
                *c = 0;
                ncmds++;
            }
        }

        // 2) Create the pipes
        int pipes[ncmds - 1][2];
        for (int i = 0; i < ncmds - 1; i++) {
            pipe(pipes[i]);
        }

        pid_t child = 0; // Wait for the last child
        for (int i = 0; i < ncmds; i++) {
            // Strip leading and trailing spaces
            while (*cmd == ' ') cmd++;
            char * end = cmd + strlen(cmd) - 1;
            while (*end == ' ') *end-- = 0;

            char * args = malloc(strlen(cmd) + 1);
            strcpy(args, cmd);

            // If this the last command, check for redirects
            char * redir = NULL;
            if (i == ncmds - 1) {
                redir = strchr(args, '>');
                if (redir) {
                    *redir = 0;
                    redir++;
                    while (*redir == ' ') redir++;
                }
            }

            // How many arguments?
            int argc = 1;
            for (char * c = args; *c != 0; c++) {
                if (*c == ' ') {
                    *c = 0;
                    argc++;
                }
            }

            // Set up argv
            char ** argv = malloc((argc + 1) * sizeof(char*));
            char * arg = args;
            for (int i = 0; i < argc; i++) {
                argv[i] = arg;
                arg += strlen(arg) + 1;
            }
            argv[argc] = NULL;

            // Fork!
            child = fork();
            if (!child) { // Child process

                // Redirect into file?
                if (redir) {
                    int fd = creat(redir);
                    dup2(fd, stdout);
                    close(fd);
                }

                // Redirect through pipes
                if (i != 0) {
                    dup2(pipes[i - 1][1], stdin);
                }

                if (i != ncmds - 1) {
                    dup2(pipes[i][0], stdout);
                }

                // Close all pipes
                for (int j = 0; j < ncmds - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                // Execute
                execve(argv[0], argv, NULL);
            }

            // Move to the next command
            cmd += strlen(cmd) + 1;
            while (*cmd == '\0') cmd++;
        }

        // Close all pipes
        for (int j = 0; j < ncmds - 1; j++) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }

        // Wait for the last child
        int status;
        waitpid(child, &status, 0);
        putchar('0' + status);
        putchar(' ');
    }

    return 0;
}
