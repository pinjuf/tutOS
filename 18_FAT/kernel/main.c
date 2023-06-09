#include "main.h"
#include "paging.h"
#include "vga.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "pic.h"
#include "ata.h"
#include "ext2.h"
#include "kbd.h"
#include "gpt.h"
#include "syscall.h"
#include "schedule.h"
#include "fat.h"

__attribute__((noreturn))
void _kmain() {
    // Set up our stack
    asm volatile(" \
        mov $0x120000, %rsp; \
        mov %rsp, %rbp; \
            "); 

    // Set up VGA
    vga_enable_cursor(0, 16);
    vga_get_cursor();

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

    kputs("KRN MN\n");

    part_t * my_part = get_part(1, 0);
    fat32fs_t * my_fs = get_fat32fs(my_part);

    void * buf = kmalloc(my_fs->root_dir.size);
    fat32_read(my_fs, &my_fs->root_dir, buf);

    char * root_ls = fat32_lsdir(my_fs, &my_fs->root_dir);

    kputs("Contents of root directory:\n");
    while (strlen(root_ls)) {
        kputs(root_ls);
        kputc(' ');

        root_ls += strlen(root_ls) + 1;
    }
    kputc('\n');

    fat_dirent83_t * test1_dirent = fat32_get_dirent_by_name(my_fs, &my_fs->root_dir, "TEST.TXT");
    kputs("Size of TEST.TXT: ");
    kputdec(test1_dirent->size);
    kputc('\n');

    void * test1_buf = kcalloc(test1_dirent->size + 1); // Null byte not included in the file
    fat32_read(my_fs, test1_dirent, test1_buf);

    kputs(test1_buf);

    kputs("KRN DN\n");

    while (1);
}
