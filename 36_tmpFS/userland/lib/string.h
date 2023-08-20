#pragma once

#include "types.h"

void memset(void * dest, uint8_t val, size_t len);
void memcpy(void * dest, void * src, size_t len);
void memmove(void * dest, void * src, size_t len);
int memcmp(void * dest, void * src, size_t len);

size_t strlen(char * s);
int strcmp(char * s1, char * s2);
int strncmp(char * s1, char * s2, size_t n);
char * strcpy(char * dest, char * src);

char * strchr(char * s, char c);
char * strstr(char * s1, char * s2);

void itoa(int n, char s[], uint8_t base);
void reverse(char s[]);

int atoi(char * s, uint8_t base);
