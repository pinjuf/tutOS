#include "schedule.h"
#include "util.h"
#include "mm.h"

process_t * processes;
process_t * current_processes = NULL;

void init_scheduling() {
    processes = kcalloc(sizeof(process_t) * MAX_PROCESSES);
}

void schedule(void * regframe_ptr) {
    int_regframe_t * rf = (int_regframe_t*)regframe_ptr;
}
