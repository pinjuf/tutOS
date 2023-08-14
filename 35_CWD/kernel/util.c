#include "main.h"
#include "kbd.h"
#include "mm.h"
#include "util.h"
#include "vesa.h"
#include <stdarg.h>

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void qemu_putc(char c) {
    if (c == '\n') outb(0xe9, '\r');
    outb(0xe9, c);
}

void qemu_puts(char * s) {
    for (; *s; s++) qemu_putc(*s);
}

void kputc(char c) {
    if (vesa_ready)
        vesa_putc(c);
    qemu_putc(c);
}

void kputs(char * s) {
    if (vesa_ready)
        vesa_puts(s);
    qemu_puts(s);
}

void kputdec(uint64_t x) {
    char buf[32];
    itoa(x, buf, 10);
    kputs(buf);
}

void kputhex(uint64_t x) {
    char buf[32];
    itoa(x, buf, 16);
    kputs(buf);
}

void memset(void * dest, uint8_t val, size_t len) {
    uint8_t *dst = (uint8_t *)dest;
    for (; len != 0; len--) *dst++ = val;
}

void memcpy(void * dest, void * src, size_t len) {
    const uint8_t * s = (const uint8_t *)src;
    uint8_t * d = (uint8_t *)dest;
    for (; len != 0; len--) *d++ = *s++;
}

void itoa(uint64_t x, char * out, uint8_t base) {
    char * p = out;
    char * low = out;

    do {
        *p++ = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[x % base];
        x /= base;
    } while (x);

    *p-- = '\0';

    while (low < p) {
        char tmp = *low;
        *low++ = *p;
        *p-- = tmp;
    }
}

void sitoa(int64_t x, char * out, uint8_t base) {
    if (x < 0) {
        *(out++) = '-';
        itoa(-x, out, base);
    } else {
        itoa(x, out, base);
    }
}

void init_pit0(uint32_t freq) {
    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0x36); // Configure mode for PIT0
    outb(0x40, divisor & 0xFF); // Low PIT0 byte
    outb(0x40, (divisor >> 8) & 0xFF); // High PIT0 byte
}

void init_pit2(uint32_t freq) {
    if (freq == 0) { // Stopping letting the signal pass through
        uint8_t tmp = inb(0x61) & 0xFC;
        outb(0x61, tmp);
        return;
    }

    uint32_t divisor = 1193180 / freq;

    outb(0x43, 0xB6); // Configure mode for PIT0
    outb(0x42, divisor & 0xFF); // Low PIT0 byte
    outb(0x42, (divisor >> 8) & 0xFF); // High PIT0 byte

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void kwarn(const char * source, const char * func, const char * msg) {
    kprintf(" < KWARN %s:%s() | %s >\n", source, func, msg);
}

void hexdump(void * ptr, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (!(i % 16)) {
            kprintf("0x%X | ", (uint64_t)ptr + i);
        }

        uint8_t val = ((uint8_t*)ptr)[i];

        if (val < 0x10)
            kputc('0');
        kputhex(val);
        kputc(' ');

        if (i%16 == 15) {
            for (size_t j = 0; j < 16; j++) {
                uint8_t val = ((uint8_t*)ptr)[i - 15 + j];
                if (val < 0x20 || val > 0x7E)
                    val = '.';
                kputc(val);
            }

            kputc('\n');
        } else if (i%16 == 7) {
            kputc(' '); // Middle delimiter
        }
    }

    if (n%16)
        kputc('\n');
}

size_t strlen(char * str) {
    size_t o = 0;
    for (;str[o];o++);
    return o;
}

void init_8042ps2() {
    // Step 1: Disable devices
    write_comm_8042ps2(0xAD);
    write_comm_8042ps2(0xA7);

    // Step 2: Clear input buffer
    if (read_status_8042ps2() & 1)
        read_data_8042ps2();

    // Step 3: Set configuration byte
    write_comm_8042ps2(0x20);
    uint8_t cfg = read_data_8042ps2();
    cfg &= ~(3 | (1<<6));

    write_comm_8042ps2(0x60);
    write_data_8042ps2(cfg);

    write_comm_8042ps2(0x20);
    cfg = read_data_8042ps2();

    if (!(cfg & (1 << 5))) {
        kwarn(__FILE__,__func__,"no dual channel ps/2");
    }

    // Step 4: Self test
    write_comm_8042ps2(0xAA);
    uint8_t st = read_data_8042ps2();
    if (st != 0x55) {
        kwarn(__FILE__,__func__,"ps/2 controller failed self test");
        return;
    }

    // Step 5: Check if there are 2 channels (again)
    write_comm_8042ps2(0xA8); // Try to enable channel two
    write_comm_8042ps2(0x20);
    cfg = read_data_8042ps2();
    if (cfg & (1 << 5)) { // Is it still disabled
        kwarn(__FILE__,__func__,"no dual channel ps/2");
    }
    write_comm_8042ps2(0xA7);

    // Step 6: Test ports
    write_comm_8042ps2(0xAB);
    st = read_data_8042ps2();
    if (st) {
        kwarn(__FILE__,__func__,"ps/2 port 1 test fail");
        return;
    }

    write_comm_8042ps2(0xA9);
    st = read_data_8042ps2();
    if (st) {
        kwarn(__FILE__,__func__,"ps/2 port 2 test fail");
        return;
    }

    // Step 7: Enable ports
    write_comm_8042ps2(0xAE);
    write_comm_8042ps2(0xA8);

    // Step 8: Reset devices
    write_ps2_port1(0xFF);
    if (read_data_8042ps2() != 0xAA) {
        kwarn(__FILE__,__func__,"ps/2 port 1 device reset fail");
        return;
    }
    write_ps2_port2(0xFF);
    if (read_data_8042ps2() != 0xAA) {
        kwarn(__FILE__,__func__,"ps/2 port 2 device reset fail");
        return;
    }


    // Step 9: Setup keyboard
    write_ps2_port1(0xF0); // Set scancode set
    write_ps2_port1(0x02); // Scancode set 2

    kbd_bitmap = kmalloc(64);

    // Step 10: Setup mouse
    write_ps2_port2(0xF4); // Enable packets
    // Step 10.5: Enable mouse scroll wheel (this is a drug trip)
    write_ps2_port2(0xF3); // Set sample rate
    write_ps2_port2(200); // 200 Hz
    write_ps2_port2(0xF3); // Set sample rate
    write_ps2_port2(100);
    write_ps2_port2(0xF3); // Set sample rate
    write_ps2_port2(80);

    write_ps2_port2(0xF2); // Check MouseID
    if (read_data_8042ps2() != 3) {
        kwarn(__FILE__,__func__,"ps/2 mouse doesn't have a scroll wheel");
        return;
    }

    // Step 11: Clear buffer (again)
    while (read_status_8042ps2() & 1)
        read_data_8042ps2();

    // Step 12: Enable IRQs for keyboard & mouse, as well as translation to Scancode set 1
    write_comm_8042ps2(0x20);
    cfg = read_data_8042ps2();
    cfg |= 3; // Keyboard & Mouse IRQs
    cfg |= 1 << 6; // Translation for Port 1
    write_comm_8042ps2(0x60);
    write_data_8042ps2(cfg);
}

uint8_t read_data_8042ps2() {
    while (!(inb(0x64) & 1));

    return inb(0x60);
}

void write_data_8042ps2(uint8_t d) {
    while (inb(0x64) & 2);

    outb(0x60, d);
}

void write_comm_8042ps2(uint8_t d) {
    while (inb(0x64) & 2);

    outb(0x64, d);
}

void write_ps2_port1(uint8_t d) {
    uint8_t r = 0;
    do {
        write_data_8042ps2(d);

        r = read_data_8042ps2();
    } while (r == PS2_RESEND);
}

void write_ps2_port2(uint8_t d) {
    uint8_t r = 0;
    do {
        write_comm_8042ps2(0xD4);
        write_data_8042ps2(d);

        r = read_data_8042ps2();
    } while (r == PS2_RESEND);
}

void kputleadingzeroes_hex(uint64_t val, uint8_t len) {
    if (val == 0) // kputhex will put a 0 anyways
        len--;

    for (uint8_t i = len - 1; i < UINT8_MAX; i--) {
        uint8_t v = val >> (i * 4) & 0xF;
        if (!v)
            kputc('0');
        else
            break;
    }
}

void _rdmsr(uint32_t msr, uint32_t * lo, uint32_t * hi) {
    asm volatile ("rdmsr" : "=a" (*lo), "=d" (*hi) : "c" (msr));
}

void _wrmsr(uint32_t msr, uint32_t lo, uint32_t hi) {
    asm volatile ("wrmsr" : : "a" (lo), "d" (hi), "c" (msr));
}

uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    _rdmsr(msr, &lo, &hi);
    return ((uint64_t)hi << 32) | lo;
}

void wrmsr(uint32_t msr, uint64_t val) {
    _wrmsr(msr, val & 0xFFFFFFFF, val >> 32);
}

int strcmp(char * s1, char * s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

int strncmp(char * s1, char * s2, size_t n) {
    char * a = s1;
    char * b = s2;

    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i])
            return a[i] - b[i];
    }

    return 0;
}

size_t strmatchstart(char * s1, char * s2) {
    size_t out = 0;

    size_t len = strlen(s1);
    if (strlen(s2) < len)
        len = strlen(s2);

    for (size_t i = 0; i < len; i++) {
        if (s1[i] == s2[i])
            out++;
        else
            break;
    }

    return out;
}

size_t atoi(char * s, uint8_t base) {
    size_t len = strlen(s);
    size_t out = 0;

    size_t factor = 1;

    for (size_t i = 0; i < len; i++) {
        char current = s[len-i-1];

        size_t val = -1;
        const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        for (size_t j = 0; j < strlen((char*)digits); j++) {
            if (current == digits[j]) {
                val = j;
                break;
            }
        }

        if (val == (size_t)-1) {
            kwarn(__FILE__,__func__,"unknown char");
        }

        out += val * factor;
        factor *= base;
    }

    return out;
}

void kprintf(char * fmt, ...) {
    // Small and basic NON STANDARD printf implementation

    va_list args;
    va_start(args, fmt);

    bool engaged = false;
    uint8_t dsize = sizeof(int);

    for (size_t i = 0; i < strlen(fmt); i++) {
        char c = fmt[i];

        if (engaged) {
            switch (c) {
                case 'h': {
                    if (dsize == sizeof(short)) {
                        dsize = sizeof(char); // double h means 8 bits
                    } else {
                        dsize = sizeof(short);
                    }
                    break;
                }
                case 'l': {
                    if (dsize == sizeof(long))
                        dsize = sizeof(long long);
                    else
                        dsize = sizeof(long);
                    break;
                }
                case 'q': {
                    dsize = sizeof(long long);
                    break;
                }
                case 'j': {
                    dsize = sizeof(intmax_t);
                    break;
                }
                case 'Z':
                case 'z': {
                    dsize = sizeof(size_t);
                    break;
                }
                case 't': {
                    dsize = sizeof(ptrdiff_t);
                    break;
                }
                case 'd':
                case 'i': { // Signed decimal integer
                    engaged = false;
                    int64_t val;
                    switch (dsize) {
                        case sizeof(int8_t):  // Google promotion
                            val = (int8_t) va_arg(args, int);
                            break;
                        case sizeof(int16_t): // Holy variadic
                            val = (int16_t) va_arg(args, int);
                            break;
                        case sizeof(int32_t): // New standard just dropped
                            val = (int32_t) va_arg(args, int32_t);
                            break;
                        case sizeof(int64_t): // Actual zombie process
                            val = (uint64_t) va_arg(args, int64_t);
                            break;
                        default:              // Call the XOR-xist!
                            val = (int) va_arg(args, int);
                            break;
                    }
                    char buf[24];
                    sitoa(val, buf, 10);
                    kputs(buf);
                    break;
                }
                case 'u': { // Unsigned integer
                    engaged = false;
                    uint64_t val = va_arg(args, uint64_t);
                    val &= SIZE_MAX >> (sizeof(size_t) * (sizeof(size_t) - dsize)); // Drop high bits
                    char buf[24];
                    itoa(val, buf, 10);
                    kputs(buf);
                    break;
                }
                case 'x': { // Hexadecimal normal
                    engaged = false;
                    uint64_t val = va_arg(args, uint64_t);
                    val &= SIZE_MAX >> (sizeof(size_t) * (sizeof(size_t) - dsize));
                    char buf[24];
                    itoa(val, buf, 16);
                    kputs(buf);
                    break;
                }
                case 'X': { // Hexadecimal padded to 64 bits
                    engaged = false;
                    uint64_t val = va_arg(args, uint64_t);
                    val &= SIZE_MAX >> (sizeof(size_t) * (sizeof(size_t) - dsize));
                    char buf[24];
                    itoa(val, buf, 16);
                    kputleadingzeroes_hex(val, 16);
                    kputs(buf);
                    break;
                }
                case 'p': { // Pointer
                    engaged = false;
                    uint64_t val = va_arg(args, uint64_t);
                    engaged = false;
                    char buf[24];
                    itoa(val, buf, 16);
                    kputs("0x");
                    kputleadingzeroes_hex(val, 16);
                    kputs(buf);
                    break;
                }
                case 's': { // String
                    engaged = false;
                    char * val = va_arg(args, char*);
                    kputs(val);
                    break;
                }
                case 'c': { // Character
                    engaged = false;
                    char val = va_arg(args, int);
                    kputc(val);
                    break;
                }
                case '%': {
                    engaged = false;
                    kputc('%');
                    break;
                }
                default: // Unknown/unsupported
                    engaged = false;
                    break;
            }

        } else if (c == '%') {
            engaged = true;
            dsize = sizeof(int); // default data size
        } else {
            kputc(c);
        }
    }
}

char * strcpy(char * dst, char * src) {
    size_t len = strlen(src);
    memcpy(dst, src, strlen(src));
    dst[len] = '\0';
    return dst;
}

char * stpcpy(char * dst, char * src) {
    size_t len = strlen(src);
    memcpy(dst, src, strlen(src));
    dst[len] = '\0';
    return &dst[len];
}

int memcmp(void * s1, void * s2, size_t n) {
    uint8_t * a = (uint8_t*) s1;
    uint8_t * b = (uint8_t*) s2;

    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i])
            return a[i] - b[i];
    }

    return 0;
}

char * strstr(char * haystack, char * needle) {
    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);

    if (needle_len > haystack_len)
        return NULL;

    for (size_t i = 0; i < haystack_len - needle_len + 1; i++) {
        if (memcmp(&haystack[i], needle, needle_len) == 0)
            return &haystack[i];
    }

    return NULL; 
}

void memmove(void * dest, void * src, size_t len) {
    uint8_t * d = (uint8_t*) dest;
    uint8_t * s = (uint8_t*) src;

    if (d < s) {
        for (size_t i = 0; i < len; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = len; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
}

size_t strreplace(char * str, char * find, char * replace) {
    // Returns the number of replacements made
    // We assume replace is shorter than find

    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);

    size_t replacements = 0;

    while (1) {
        char * found = strstr(str, find);
        if (!found)
            break;

        memmove(found + replace_len, found + find_len, strlen(found + find_len) + 1);

        memcpy(found, replace, replace_len);

        replacements++;
    }

    return replacements;
}
