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

    // Set DSP IRQ
    outb(DSP_MIXER, 0x80);
    outb(DSP_MIXER, 0x02); // IRQ5

    // DMA initialization is done on sb16_play()
}

void sb16_volume(uint8_t volume) {
    // Upper 4 bits are left, lower are right

    outb(DSP_MIXER, 0x22);
    outb(DSP_MIXER_DATA, volume);
}

// We assume we only play 8-bit signed mono 44.1kHz sound
void sb16_start_play() {
    outb(DSP_RESET, 1);
    for (uint8_t i = 0; i < 32; i++)
        asm volatile ("nop");
    outb(DSP_RESET, 0);

    if (sb16_player->_16bit) {
        dma_chan_init(5, DSP_BUF, DSP_SIZE);
        dma_chan_mode(5, DMA_SINGLE | DMA_AUTOINIT | DMA_TRA_READ);
    } else {
        dma_chan_init(1, DSP_BUF, DSP_SIZE);
        dma_chan_mode(1, DMA_SINGLE | DMA_AUTOINIT | DMA_TRA_READ);
    }

    // Manually trigger the loader
    isr_irq5();
}

void isr_irq5() {
    // Acknowledge IRQs (if any)
    inb(DSP_IRQACK8);
    inb(DSP_IRQACK16);

    if (!sb16_player->playing) {
        // We're done
        return;
    }

    sb16_volume(sb16_player->volume);

    // Turn the speakers on
    outb(DSP_WRITE, 0xD1);

    uint32_t to_play = DSP_SIZE;
    if (sb16_player->current + to_play > sb16_player->size) {
        to_play = sb16_player->size - sb16_player->current;
    }

    memcpy(DSP_BUF, (void*)((uint64_t)sb16_player->data + sb16_player->current), to_play);

    uint8_t chans = sb16_player->stereo ? 2 : 1;

    uint8_t transfer_mode = DSP_TRA8;
    if (sb16_player->_16bit)
        transfer_mode = DSP_TRA16;

    uint8_t sound_type = 0;
    if (sb16_player->stereo)
        sound_type |= DSP_STEREO;
    if (sb16_player->sign)
        sound_type |= DSP_SIGNED;

    outb(DSP_WRITE, 0x40);
    outb(DSP_WRITE, DSP_TO_TIMECONSTANT(sb16_player->sampling_rate, chans));
    outb(DSP_WRITE, transfer_mode);
    outb(DSP_WRITE, sound_type);

    outb(DSP_WRITE, (to_play-1) & 0xFF);
    outb(DSP_WRITE, ((to_play-1) >> 8 & 0xFF));

    sb16_player->current += to_play;
    if (sb16_player->current >= sb16_player->size) {
        sb16_player->playing = false;
    }
}
