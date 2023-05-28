#pragma once

#include "types.h"

#include "unistd.h"
#include "libtut.h"

int puts(char * s);
int putc(char c);
int fputs(char * s, FILE * f);
int fputc(char c, FILE * f);
