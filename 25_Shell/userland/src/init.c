#include "unistd.h"

// Length: 20
char * msg = "Hello from usermode!\n";

int main() {
    FILE * file = open("/dev/tty", FILE_W);
    write(file, msg, 21);
    close(file);
}
