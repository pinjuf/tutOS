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

char * strcpy(char * dest, char * src) {
    char * ret = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return ret;
}

int strncmp(char * s1, char * s2, size_t n) {
    while (n > 0 && *s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }

    if (n == 0) return 0;

    return *s1 - *s2;
}

// Shamelessly stolen from stackoverflow 190229
void itoa(int n, char s[], uint8_t base) {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = "0123456789ABCDEF" [n % base];   /* get next digit */
     } while ((n /= base) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
}

void reverse(char s[]) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
       s[j] = c;
    }
}

int atoi(char * s, uint8_t base) {
    int n = 0;
    int i = 0;
    int sign = 1;

    if (s[0] == '-') {
        sign = -1;
        i++;
    }

    for (; s[i] != '\0'; i++) {
        n = n * base + s[i] - '0';
    }

    return sign * n;
}
