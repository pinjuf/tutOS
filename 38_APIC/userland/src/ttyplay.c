#include "unistd.h"
#include "stdio.h"
#include "types.h"
#include "stdlib.h"

typedef struct {
    uint32_t sec;
    uint32_t usec;
    uint32_t len;
} __attribute__((packed)) header_t;

int main(int argc, char * argv[]) {
    // TTYREC player

    header_t header;

    if (argc != 2) {
        puts("Usage: ttyplay <file>\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        puts("ttyplay: can't open file\n");
    }

    size_t passed_time = SIZE_MAX; // [ms]

    while (read(fd, &header, sizeof(header)) == sizeof(header)) {
        if (passed_time == SIZE_MAX) { // First frame
            passed_time = header.sec * 1000 + header.usec / 1000;
        }

        size_t time = header.sec * 1000 + header.usec / 1000;
        pit_msleep(time - passed_time);
        passed_time = time;

        char * buf = malloc(header.len);
        read(fd, buf, header.len);
        write(1, buf, header.len);
        free(buf);
    }

    close(fd);
}
