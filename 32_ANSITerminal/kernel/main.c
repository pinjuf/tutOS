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
    vesa_clear(vfont_bg);
    kputs("VBE OK\n");

    init_sb16();
    kputs("SB16 OK\n");

    kputs("KRN MN\n");

    filehandle_t * init_fh = kopen(INIT_PATH, O_RDONLY);
    if (!init_fh) {
        kwarn(__FILE__,__func__,"no init executable found");
    }

    for (size_t i = 0; i < 8; i++) {
        rgb32_t c = col256_to_rgb32(i);
        vfont_bg = c;
        kputs("        ");
    }
    kputc('\n');

    for (size_t i = 0; i < 8; i++) {
        rgb32_t c = col256_to_rgb32(8 + i);
        vfont_bg = c;
        kputs("        ");
    }
    kputc('\n');

    for (size_t j = 0; j < 6; j++) {
        for (size_t i = 0; i < 6*6; i++) {
            rgb32_t c = col256_to_rgb32(16 + j * 6 * 6 + i);
            vfont_bg = c;
            kputs("  ");
        }
        kputc('\n');
    }

    for (size_t i = 0; i < 24; i++) {
        rgb32_t c = col256_to_rgb32(232 + i);
        vfont_bg = c;
        kputs("   ");
    }
    kputs("\033[m\n");

    char * buf = kmalloc(init_fh->size);
    kread(init_fh, buf, init_fh->size);
    kclose(init_fh);
    process_t * init_proc = add_process();
    init_proc->parent = KERN_PID; // Kernel is the parent
    elf_load(init_proc, buf, 0x10, false);
    init_proc->state = PROCESS_RUNNING;
    proc_set_args(init_proc, 0, NULL);
    kfree(buf);

    do_scheduling = true;
    sti;

    while (1);
}
