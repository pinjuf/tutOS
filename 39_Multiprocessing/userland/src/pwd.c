#include "unistd.h"
#include "stdio.h"

int main(int argc, char * argv[]) {
    char buf[1024];
    if (getcwd(buf, sizeof(buf)) < 0) {
        puts("getcwd failed");
    }
    puts(buf);
    putchar('\n');
    return 0;
}
