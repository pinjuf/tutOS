#include "isr.h"
#include "kbd.h"
#include "main.h"
#include "schedule.h"
#include "util.h"
#include "signal.h"
#include "apic.h"
#include "ap.h"

volatile uint64_t pit0_ticks = 0;

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;
    (void)ss;

    cpu_coreinfo_t * core = get_core();

    if (core->current_process && (cs & 3)) {
        // This was caused by a process
        // that will now have to answer
        // for its crimes

        kprintf(" < PEXC %u (SIG=%d) AT 0x%x BY PID#%u (core #%hhu)>\n", n, EXCEPTION_SIGNALS[n], rip, core->current_process->pid, core->apic_id);

        push_proc_sig(core->current_process, EXCEPTION_SIGNALS[n]);

        // Trigger a manual schedule
        core->current_process = NULL;
        asm volatile ("int $0x82");
    }

    // Kernel error with no process running
    kprintf(" < KEXC %u AT 0x%x (core #%hhu)>\n", n, rip, core);

    while (1);
}

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss) {
    (void)n;
    (void)err;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;
    (void)ss;

    cpu_coreinfo_t * core = get_core();

    if (core->current_process && (cs & 3)) {
        // This was caused by a process
        // that will now have to answer
        // for its crimes

        kprintf(" < PEXC %u (ERR=0x%x, SIG=%d) AT 0x%x BY PID#%u (core #%hhu)>\n", n, err, EXCEPTION_SIGNALS[n], rip, core->current_process->pid, core);

        push_proc_sig(core->current_process, EXCEPTION_SIGNALS[n]);

        // Trigger a manual schedule
        core->current_process = NULL;
        asm volatile ("int $0x82");
    }

    // Kernel error with no process running
    kprintf(" < KEXC %u (ERR=0x%x) AT 0x%x (core #%hhu)>\n", n, err, rip, core);

    while (1);
}

void isr_default_int(uint16_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss) {
    (void)n;
    (void)rip;
    (void)cs;
    (void)rflags;
    (void)rsp;
    (void)ss;

    kprintf("INT ");
    if (n == 0xFFFF)
        kprintf("[UNKN]");
    else
        kprintf("%u", n);
    kprintf(" AT 0x%x\n", rip);
}

void isr_irq0(int_regframe_t * regframe) {
    pit0_ticks++;

    if (!(pit0_ticks % TICKS_PER_SCHEDULE) && do_scheduling) {
        schedule_ticks++;

        // Every scheduling tick, a core is chosen to reschedule
        cpu_coreinfo_t * core = &coreinfos[schedule_ticks % cpu_cores];

        if (core->bsp) { // Only the BSP catches an IRQ0
            //schedule(regframe);
        } else {
            apic_ipi(core->apic_id, 0x82, 0);
        }
    }

    if (!(pit0_ticks % PROC_CLEANER_TICKS) && do_scheduling) {
        clear_none_procs();
    }
}

void isr_irq1(void) {
    // The keyboard uses SCS2, but the 8042 translates it to SCS1 for us

    // Because we read multiple times in this ISR, the 8042 will trigger
    // multiple interrupts WHILE we are in this ISR, even when we have
    // already read the data corresponding to the IRQ. Therefore, the
    // moment we send the EOI, the 8259 will send the next IRQ.
    // So, we need to check if there really is data to be read.
    if (!(read_status_8042ps2() & 1)) // Is there even data to be read?
        return;

    uint8_t c = read_data_8042ps2();
    bool release = false;
    bool special = false;

    // Translation may cause data mangling, which SHOULDN'T be a problem
    if (c == 0x00) // || // Key detection error/internal buffer overrun
        //c == 0xAA || // Self test pass
        //c == 0xEE || // Echo
        //c == 0xFA || // ACK
        //c == 0xFC || // Self test fail
        //c == 0xFD || // Self test fail
        //c == 0xFE || // Resend
        //c == 0xFF)   // Key detection error/internal buffer overrun
    {
        while (read_status_8042ps2() & 1)
            read_data_8042ps2();

        return;
    }

    if (c == 0xE0) { // Special key
        special = true;
        c = read_data_8042ps2();
    }

    if (c & 0x80) {
        release = true;
        c &= ~0x80;
    }

    // Update the scancode-kbd-bitmap
    kbd_setkey(c + special * 256, !release);

    // Update the last-pressed-ascii-key-handle
    if (!(release || special)) {
        bool is_shift = kbd_getkey(SCS1_LSHIFT) || kbd_getkey(SCS1_RSHIFT);

        if (scancode_to_ascii(c)) {
            kbd_last_ascii = is_shift ? scancode_shift_to_ascii(c) : scancode_to_ascii(c);
        }
    }

    if (!release) {
        kbd_last_scancode = c + special * 256;
    }

    // Safeguard key combination, CTRL+SHIFT+C SIGKILLs all processes except the init, CTRL+ALT+C SIGTERMs all processes except the init
    if (!release && !special && kbd_getkey(SCS1_LSHIFT) && kbd_getkey(SCS1_CTRL) && scancode_to_ascii(c) == 'c') {
        for (size_t i = 0; i < ll_len(processes); i++) {
            process_t * proc = ll_get(processes, i);
            if (proc->pid == INIT_PID || !IS_ALIVE(proc))
                continue;

            push_proc_sig(proc, SIGKILL);

            kprintf(" < SENT SIGKILL TO #%u > ", proc->pid);
        }

        kbd_last_scancode = 0;
        kbd_last_ascii    = 0;
    }

    if (!release && !special && kbd_getkey(SCS1_ALT) && kbd_getkey(SCS1_CTRL) && scancode_to_ascii(c) == 'c') {
        for (size_t i = 0; i < ll_len(processes); i++) {
            process_t * proc = ll_get(processes, i);
            if (proc->pid == INIT_PID || !IS_ALIVE(proc))
                continue;

            push_proc_sig(proc, SIGTERM);

            kprintf(" < SENT SIGTERM TO #%u > ", proc->pid);
        }

        kbd_last_scancode = 0;
        kbd_last_ascii    = 0;
    }

    // Just to be sure, clear the 8042 output buffer
    while (read_status_8042ps2() & 1)
        read_data_8042ps2();
}

void isr_irq12(void) {
    if (!(read_status_8042ps2() & 1)) // Is there even data to be read?
        return;
    uint8_t mouse_flags = read_data_8042ps2();

    if (!(read_status_8042ps2() & 1)) // Make sure the X info isn't missing
        return;
    uint8_t x_raw = read_data_8042ps2();

    if (!(read_status_8042ps2() & 1)) // Make sure the Y info isn't missing
        return;
    uint8_t y_raw = read_data_8042ps2();

    if (!(read_status_8042ps2() & 1)) // Make sure the Z (scroll) info isn't missing
        return;
    uint8_t scroll_raw = read_data_8042ps2() & 0xF; // Drop top 4 bits

    int16_t true_dx = x_raw;
    int16_t true_dy = y_raw;

    if (mouse_flags & PS2_MOUSE_XSIGN)
        true_dx -= 256;
    if (mouse_flags & PS2_MOUSE_YSIGN)
        true_dy -= 256;

    mouse_left = (mouse_flags & PS2_MOUSE_LEFT) != 0;
    mouse_middle = (mouse_flags & PS2_MOUSE_MDDL) != 0;
    mouse_right = (mouse_flags & PS2_MOUSE_RGHT) != 0;

    mouse_x += true_dx * MOUSE_XSCALE;
    mouse_y += true_dy * MOUSE_YSCALE;

    if (mouse_x > mouse_xlim)
        mouse_x = mouse_xlim;
    if (mouse_y > mouse_ylim)
        mouse_y = mouse_ylim;

    if (mouse_x < 0)
        mouse_x = 0;
    if (mouse_y < 0)
        mouse_y = 0;

    if (scroll_raw)
        mouse_scroll = scroll_raw;

    while (read_status_8042ps2() & 1)
        read_data_8042ps2();
}

void isr_debugcall(int_regframe_t * regframe) {
    kprintf("< DEBUGCALL START >\n");

    kprintf("RIP = 0x%X RSP = 0x%X CS=0x%x SS=0x%x RFLAGS=0x%X\n", regframe->rip, regframe->rsp, regframe->cs, regframe->ss, regframe->rflags);

    kprintf("RBP = 0x%X GS = 0x%x FS=0x%x CR3=0x%X\n", regframe->rbp, regframe->gs, regframe->fs, regframe->cr3);

    kprintf("RAX = 0x%X RBX = 0x%X RCX = 0x%X RDX = 0x%X\n", regframe->rax, regframe->rbx, regframe->rcx, regframe->rdx);

    kprintf(" R8 = 0x%X  R9 = 0x%X R10 = 0x%X R11 = 0x%X\n", regframe->r8, regframe->r9, regframe->r10, regframe->r11);

    kprintf("R12 = 0x%X R13 = 0x%X R14 = 0x%X R15 = 0x%X\n", regframe->r12, regframe->r13, regframe->r14, regframe->r15);

    kprintf("RSI = 0x%X RDI = 0x%X\n", regframe->rsi, regframe->rdi);

    kprintf("< DEBUGCALL END >\n");
}

void isr_schedule(int_regframe_t * rf) {
    // Warning: THIS IS DIRTY AND WILL CAUSE LOSS OF EXECUTION IF A PROCESS IS RUNNING AS WELL AS A MICRO-TIMESHIFT!
    schedule(rf);
}

void isr_spurious(void) {
    return;
}
