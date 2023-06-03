#include "dma.h"
#include "util.h"

int dma_chan_init(uint8_t chan, void * addr, uint16_t count) {
    if (chan == 0 || chan == 4 || chan >= 8) {
        kwarn(__FILE__,__func__,"invalid dma channel");
        return 1;
    }

    if ((uint64_t)addr >> 24) {
        kwarn(__FILE__,__func__,"mem out of range");
    }

    uint8_t master = chan & 4 ? 1 : 0;

    // "local" channel number for 8237A
    uint8_t _chan = chan & 3;

    // Mask our channel
    outb(DMA_SINGLE_MASK[master], 4 | _chan);

    // Reset the master flip flop (we will be sending 2 bytes of addr data)
    outb(DMA_FLIP_FLOP[master], 0xFF);

    // Physical Address (low byte first)
    outb(DMA_ADDRESS[chan], (uint64_t)addr & 0xFF);
    outb(DMA_ADDRESS[chan], ((uint64_t)addr >> 8) & 0xFF);

    outb(DMA_FLIP_FLOP[master], 0xFF);

    // Count (low byte first)
    outb(DMA_COUNT[chan], count & 0xFF);
    outb(DMA_COUNT[chan], count >> 8);

    // Unmask our channel
    outb(DMA_SINGLE_MASK[master], _chan);

    return 0;
}
