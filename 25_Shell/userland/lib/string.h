#pragma once

#include "types.h"

void memset(void * dest, uint8_t val, size_t len);
void memcpy(void * dest, void * src, size_t len);

size_t strlen(char * s);
int strcmp(char * s1, char * s2);
int strncmp(char * s1, char * s2, size_t n);
