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
    ext2fs_t * my_fs = get_ext2fs(my_part);

    ext2_inode_t * root_dir = get_inode(my_fs, 2);

    char * root_ls = ext2_lsdir(my_fs, root_dir);

    kputs("Listing files in root:\n");
    while (strlen(root_ls)) {
        kputs(root_ls);
        kputc(' ');

        root_ls += strlen(root_ls) + 1;
    }
    kputc('\n');

    uint32_t test_index = ext2_get_inode(my_fs, root_dir, "test.txt");
    kputs("Inode N of test.txt: ");
    kputdec(test_index);
    kputc('\n');

    ext2_inode_t * test_inode = get_inode(my_fs, test_index);
    kputs("Size of test.txt: ");
    kputdec(test_inode->i_size);
    kputc('\n');

    char * buf = (char *) kmalloc(test_inode->i_size + 1); // Null byte is not included in text file
    buf[test_inode->i_size] = 0;
    ext2_read_inode(my_fs, test_inode, buf);
    kputs("Contents of test.txt:\n");
    kputs(buf);

    kputs("KRN DN\n");

    while (1);
}
