#include "main.h"
#include "paging.h"
#include "util.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
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
#include "fd.h"
#include "pipe.h"
#include "rnd.h"
#include "acpi.h"
#include "apic.h"
#include "ap.h"

bpob_t * bpob = (void*)BPOB_ADDR;

size_t cpu_tps = 0;

__attribute__((noreturn))
void _kmain() {
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

    init_acpi();
    kprintf("ACPI OK\n");

    init_apic();
    kprintf("APIC OK\n");

    init_pit0(PIT0_FREQ);
    kprintf("PIT OK\n");

    sti;
    cpu_tps = get_cpu_tps(); // Calibrate CPU Ticks per Second
    cli;

    init_8042ps2();
    kprintf("PS2 OK\n");

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

    init_ap();
    kprintf("SMP OK\n");

    init_sb16();
    kprintf("SB16 OK\n");

    kprintf("KRN MN\n");

    // Init process
    filehandle_t * init_fh = kopen(INIT_PATH, O_RDONLY);
    if (!init_fh) {
        kwarn(__FILE__,__func__,"no init executable found");
    }

    // Init process initialization
    // Process loading
    char * buf = kmalloc(init_fh->size);
    kread(init_fh, buf, init_fh->size);
    kclose(init_fh);
    process_t * init_proc = add_process();
    init_proc->parent = KERN_PID; // Kernel is the parent
    elf_load(init_proc, buf, 0x10, false);
    init_proc->state = PROCESS_RUNNING;
    proc_set_args(init_proc, 0, NULL);
    kfree(buf);

    // IO setup
    filehandle_t * i_stdin  = kopen("/dev/tty", O_RDONLY);
    filehandle_t * i_stdout = kopen("/dev/tty", O_WRONLY);
    filehandle_t * i_stderr = kopen("/dev/tty", O_WRONLY);

    fd_t * fd_stdin  = add_fd(init_proc); fd_stdin->n  = stdin;
    fd_t * fd_stdout = add_fd(init_proc); fd_stdout->n = stdout;
    fd_t * fd_stderr = add_fd(init_proc); fd_stderr->n = stderr;

    fd_stdin->type  = FD_VFS;
    fd_stdout->type = FD_VFS;
    fd_stderr->type = FD_VFS;

    fd_stdin->handle  = i_stdin;
    fd_stdout->handle = i_stdout;
    fd_stderr->handle = i_stderr;

    i_stdin->fd_refs++;
    i_stdout->fd_refs++;
    i_stderr->fd_refs++;

    init_proc->fd_n = 3;

    proc_set_cwd(init_proc, "/");

    do_scheduling = true;

    sti;
    while (1);
}
