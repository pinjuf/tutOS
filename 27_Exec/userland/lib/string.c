#include "string.h"

void memset(void * dest, uint8_t val, size_t len) {
    uint8_t *dst = (uint8_t *)dest;
    for (; len != 0; len--) *dst++ = val;
}

void memcpy(void * dest, void * src, size_t len) {
    const uint8_t * s = (const uint8_t *)src;
    uint8_t * d = (uint8_t *)dest;
    for (; len != 0; len--) *d++ = *s++;
}

size_t strlen(char * str) {
    size_t o = 0;
    for (;str[o];o++);
    return o;
}

int strcmp(char * s1, char * s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}
