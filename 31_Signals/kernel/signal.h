#pragma once

#include "types.h"
#include "schedule.h"

#include "../lib/signal.h"

void push_proc_sig(process_t * proc, int sig);
int pop_proc_sig(process_t * proc);
struct sigaction * get_proc_sigaction(process_t * proc, int sig);
void register_sigaction(process_t * proc, struct sigaction * action);
void manual_schedule();
void del_proc_sig(process_t * proc, size_t index);
