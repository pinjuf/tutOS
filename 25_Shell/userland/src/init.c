#include "unistd.h"
#include "stdio.h"

// Length: 20
char * msg = "Hello from usermode!\n";

int main() {
    puts(msg);
}
