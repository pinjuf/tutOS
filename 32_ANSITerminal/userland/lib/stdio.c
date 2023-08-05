#include "stdio.h"
#include "string.h"

int puts(char * s) {
    return write(stdout, s, strlen(s));
}

int putc(char c) {
    return write(stdout, &c, 1);
}

int fputs(char * s, FILE * f) {
    return write(f, s, strlen(s));
}

int fputc(char c, FILE * f) {
    return write(f, &c, 1);
}
