#include "main.h"
#include "paging.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"
#include "ata.h"
#include "kbd.h"
#include "gpt.h"
#include "syscall.h"
#include "schedule.h"
#include "vfs.h"
#include "vesa.h"
#include "elf.h"
#include "isr.h"

bpob_t * bpob = (void*)BPOB_ADDR;

__attribute__((noreturn))
void _kmain() {
    // Set up our stack
    asm volatile(" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    // I really like cool sounding logs
    kputs("KRN OK\n");

    init_kgdt();
    kputs("GDT OK\n");

    init_idt();
    kputs("IDT OK\n");

    init_paging();
    kputs("PGN OK\n");

    init_mm();
    kputs("MM  OK\n");

    init_pic();
    kputs("PIC OK\n");

    init_pit0(PIT0_FREQ);
    kputs("PIT OK\n");

    init_8042ps2();
    kputs("PS2 OK\n");

    pic_setmask(pic_getmask() | (3<<14)); // Disable Both ATA interrupts
    for (uint8_t i = 0; i < 8; i++) {
        ata_resetdrive(i);
    }
    ata_checkdrives();
    kputs("ATA OK\n");

    init_syscalls();
    kputs("SCL OK\n");

    init_scheduling();
    kputs("SCD OK\n");

    init_vfs();
    kputs("VFS OK\n");

    init_vesa();
    vesa_clear(RGB32(0, 0, 0));
    kputs("VBE OK\n");

    kputs("KRN MN\n");

    for (size_t y = 0; y < 400; y++)
        for (size_t x = 0; x < 600; x++) {
            long double zr = MAP((long double)x, 0L, 600L, -2.03L, 0.5L);
            long double zi = MAP((long double)y, 0L, 400, 1.2L, -1.2L);   // Complex axis is mirrored

            long double cr = zr;
            long double ci = zi;

            for (size_t i = 0; i < 256; i++) {
                long double ocr = cr;
                cr = cr * cr - ci * ci + zr;
                ci = 2 * ocr * ci + zi;

                if (cr * cr + ci * ci > 4) {
                    SET_PIXEL(x, y, RGB32(255, 255, 255));
                    break;
                }
            }
        }

    kputs("KRN DN\n");
    while (1);
}
