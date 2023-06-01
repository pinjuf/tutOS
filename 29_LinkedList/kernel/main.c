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

    ll_head * my_list = create_ll(sizeof(process_t));

    kputs("Length after creation: ");
    kputdec(ll_len(my_list));
    kputc('\n');

    process_t * elems[3];

    elems[0] = ll_push(my_list);
    elems[0]->state = 0;
    kputs("Length after adding elem0: ");
    kputdec(ll_len(my_list));
    kputc('\n');

    elems[1] = ll_push(my_list);
    elems[1]->state = 1;
    kputs("Length after adding elem1: ");
    kputdec(ll_len(my_list));
    kputc('\n');

    elems[2] = ll_push(my_list);
    elems[2]->state = 2;
    kputs("Length after adding elem2: ");
    kputdec(ll_len(my_list));
    kputc('\n');

    for (uint8_t i = 0; i < 3; i++) {
        kputs("Testing element #");
        kputdec(i);
        kputs(": ");

        if (ll_get(my_list, i) == elems[i]) {
            kputs("PASS\n");
        } else
            kputs("FAIL\n");
    }

    ll_del(my_list, 1);
    kputs("Length after deleting elem1: ");
    kputdec(ll_len(my_list));
    kputc('\n');

    kputs("Checking that elem2 == my_list[1] after deletion: ");
    if (ll_get(my_list, 1) == elems[2]) {
        kputs("PASS\n");
    } else
        kputs("FAIL\n");

    filehandle_t * init_fh = kopen("/bin/init", FILE_R);
    if (!init_fh) {
        kwarn(__FILE__,__func__,"no init executable found");
    }

    char * buf = kmalloc(init_fh->size);
    kread(init_fh, buf, init_fh->size);
    kclose(init_fh);
    elf_load(&processes[0], buf, 0x10, false);
    proc_set_args(&processes[0], 0, NULL);
    kfree(buf);

    do_scheduling = true;
    sti;

    while (1);
}
