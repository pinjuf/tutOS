#include "stdio.h"
#include "string.h"

int puts(char * s) {
    return write(stdout, s, strlen(s));
}

int putchar(char c) {
    return write(stdout, &c, 1);
}

int fputs(char * s, int f) {
    return write(f, s, strlen(s));
}

int fputc(char c, int f) {
    return write(f, &c, 1);
}
