#include "sb16.h"
#include "dma.h"
#include "util.h"
#include "mm.h"

sb16_player_t * sb16_player;

void init_sb16() {
    // Reset the SB16 and check if it responds
    outb(DSP_RESET, 1);
    for (uint8_t i = 0; i < 32; i++)
        asm volatile ("nop");
    outb(DSP_RESET, 0);

    if (inb(DSP_READ) != 0xAA) {
        kwarn(__FILE__,__func__,"invalid dsp resp");
        return;
    }

    outb(DSP_WRITE, 0xE1); // GET DSP VERSION
    uint8_t v_major = inb(DSP_READ);
    inb(DSP_READ);

    if (v_major < 4) {
        kwarn(__FILE__,__func__,"wrong dsp version");
        return;
    }

    sb16_player = kmalloc(sizeof(sb16_player_t));

    // DMA initialization is done on sb16_play()
}

void sb16_volume(uint8_t volume) {
    // Upper 4 bits are left, lower are right

    outb(DSP_MIXER, 0x22);
    outb(DSP_MIXER_DATA, volume);
}

// We assume we only play 8-bit signed mono 44.1kHz sound
void sb16_play() {
    outb(DSP_RESET, 1);
    for (uint8_t i = 0; i < 32; i++)
        asm volatile ("nop");
    outb(DSP_RESET, 0);

    dma_chan_init(1, DSP_BUF, DSP_SIZE);
    dma_chan_mode(1, DMA_SINGLE | DMA_AUTOINIT | DMA_TRA_READ);

    // Turn the speakers on
    outb(DSP_WRITE, 0xD1);

    outb(DSP_WRITE, 0x40);
    outb(DSP_WRITE, DSP_TO_TIMECONSTANT(44100));
    outb(DSP_WRITE, DSP_TRA8);
    outb(DSP_WRITE, DSP_SIGNED);
    outb(DSP_WRITE, (DSP_SIZE-1) & 0xFF);
    outb(DSP_WRITE, (DSP_SIZE-1) >> 8);
}
