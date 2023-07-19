#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

int main(int argc, char * argv[]) {
    puts("Hello from test program!\n");

    pit_msleep(5000);
    asm ("cli"); // Causes a #GP
    pit_msleep(5000);

    puts("Bye from test program!\n");

    return 0;
}
