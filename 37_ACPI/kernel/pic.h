#pragma once

#include "types.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_DATA (PIC1+1)
#define PIC2_DATA (PIC2+1)
#define PIC_EOI 0x20

#define ICW1_ICW4 (1 << 0) // ICW4 Present
#define ICW1_SING (1 << 1) // Single mode
#define ICW1_INT4 (1 << 2) // Call address interval 4
#define ICW1_LVL  (1 << 3) // Level triggered mode
#define ICW1_INIT (1 << 4) // Initialization

#define ICW4_8086 (1 << 0) // 8086/88 mode
#define ICW4_AUTO (1 << 1) // Auto EOI
#define ICW4_BUFS (1 << 3) // Buffered slave mode
#define ICW4_BUFM (0xC)    // Buffered master mode
#define ICW4_SFNM (1 << 4) // Special fully nested

#define PIC_OFFSET 0x20
enum {
    PIC_PIT = 0,
    PIC_KBD,
    PIC_CASCADE,
    PIC_COM2,
    PIC_COM1,
    PIC_LPT2,
    PIC_DSP = 5,
    PIC_FLOPPY,
    PIC_LPT1,
    PIC_CMOS,
    PIC_FREE1,
    PIC_FREE2,
    PIC_FREE3,
    PIC_MOUSE,
    PIC_FPU,
    PIC_ATA_P,
    PIC_ATA_S,
};

void init_pic(void);

void pic_setmask(uint16_t mask);
uint16_t pic_getmask();
