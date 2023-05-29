#pragma once

#include "types.h"
#include "schedule.h"

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss);

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss);

void isr_default_int(uint16_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp, uint64_t ss);

void isr_irq0(int_regframe_t * regframe);
void isr_irq1(void);
void isr_irq12(void);

void isr_debugcall(int_regframe_t * regframe);

extern uint64_t pit0_ticks;
