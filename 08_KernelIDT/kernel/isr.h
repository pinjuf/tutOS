#pragma once

#include "types.h"

void isr_noerr_exception(uint8_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp);

void isr_err_exception(uint8_t n, uint64_t err, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp);

void isr_default_int(uint16_t n, uint64_t rip, uint64_t cs, uint64_t rflags, uint64_t rsp);
