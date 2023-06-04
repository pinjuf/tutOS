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
#include "ll.h"
#include "sb16.h"

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

    init_sb16();
    kputs("SB16 OK\n");

    kputs("KRN MN\n");

    int16_t * audiobuf = kmalloc(44100 * 2);
    int16_t * curr = audiobuf;

    sb16_player->sign          = true;
    sb16_player->size          = 44100 * 2;
    sb16_player->data          = audiobuf;
    sb16_player->current       = 0;
    sb16_player->stereo        = false;
    sb16_player->_16bit        = true;
    sb16_player->volume        = 0x11;
    sb16_player->playing       = true;
    sb16_player->sampling_rate = 44100;

    for (int i = 0; i < 440; i++) {
        for (int j = 0; j < 50; j++) {
            *curr++ = INT16_MIN;
        }
        for (int j = 0; j < 50; j++) {
            *curr++ = INT16_MAX;
        }
    }

    sb16_start_play();

    filehandle_t * init_fh = kopen("/bin/init", FILE_R);
    if (!init_fh) {
        kwarn(__FILE__,__func__,"no init executable found");
    }

    char * buf = kmalloc(init_fh->size);
    kread(init_fh, buf, init_fh->size);
    kclose(init_fh);
    process_t * init_proc = add_process();
    init_proc->parent = 0; // 0 means the kernel is the parent
    elf_load(init_proc, buf, 0x10, false);
    proc_set_args(init_proc, 0, NULL);
    kfree(buf);

    do_scheduling = true;
    sti;

    while (1);
}
