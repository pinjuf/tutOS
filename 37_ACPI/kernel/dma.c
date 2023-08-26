#include "dma.h"
#include "util.h"

int dma_chan_init(uint8_t chan, void * addr, uint32_t count) {
    // Count is the actual number of transfered 8-bit bytes

    // IDK if channel 0 really is unavailable,
    // but the OSDev wiki's article says so...
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

    // Some redundant ANDS, I know...

    uint16_t addr_low = (uint64_t)addr & 0xFFFF;
    uint8_t page = ((uint64_t)addr >> 16) & 0xFF;


    // The 16-bit chip (master) is HARDWIRED so that its
    // output address is shifted one bit to the LEFT.
    // If it increases its internal counter by one,
    // the output is increased by two (hence its 16-bit usage).
    if (chan >= 4) {
        addr_low >>= 1;
        count >>= 1;
    }

    count--;

    // Mask our channel
    outb(DMA_SINGLE_MASK[master], 4 | _chan);

    // Reset the master flip flop (we will be sending 2 bytes of addr data)
    outb(DMA_FLIP_FLOP[master], 0xFF);

    // Physical Address (low byte first)
    outb(DMA_ADDRESS[chan], addr_low & 0xFF);
    outb(DMA_ADDRESS[chan], (addr_low >> 8) & 0xFF);

    outb(DMA_PAGE[chan], page);

    outb(DMA_FLIP_FLOP[master], 0xFF);

    // Count (low byte first)
    outb(DMA_COUNT[chan], count & 0xFF);
    outb(DMA_COUNT[chan], count >> 8);

    // Unmask our channel
    outb(DMA_SINGLE_MASK[master], _chan);

    return 0;
}

int dma_chan_mode(uint8_t chan, uint8_t mode) {

    uint8_t master = chan & 4 ? 1 : 0;
    uint8_t _chan = chan & 3;

    outb(DMA_SINGLE_MASK[master], 4 | _chan);

    outb(DMA_MODE[master], mode | _chan);

    outb(DMA_SINGLE_MASK[master], _chan);

    return 0;
}
