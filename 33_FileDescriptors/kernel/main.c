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
    kprintf("KRN OK\n");

    init_kgdt();
    kprintf("GDT OK\n");

    init_idt();
    kprintf("IDT OK\n");

    init_paging();
    kprintf("PGN OK\n");

    init_mm();
    kprintf("MM  OK\n");

    init_pic();
    kprintf("PIC OK\n");

    init_pit0(PIT0_FREQ);
    kprintf("PIT OK\n");

    init_8042ps2();
    kprintf("PS2 OK\n");

    pic_setmask(pic_getmask() | (3<<14)); // Disable Both ATA interrupts
    for (uint8_t i = 0; i < 8; i++) {
        ata_resetdrive(i);
    }
    ata_checkdrives();
    kprintf("ATA OK\n");

    init_syscalls();
    kprintf("SCL OK\n");

    init_scheduling();
    kprintf("SCD OK\n");

    init_vfs();
    kprintf("VFS OK\n");

    init_vesa();
    vesa_clear(vfont_bg);
    kprintf("VBE OK\n");

    init_sb16();
    kprintf("SB16 OK\n");

    kprintf("KRN MN\n");

    filehandle_t * init_fh = kopen(INIT_PATH, O_RDONLY);
    if (!init_fh) {
        kwarn(__FILE__,__func__,"no init executable found");
    }

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
