#include "schedule.h"
#include "util.h"

void schedule(void * regframe) {
    kputhex((uint64_t)regframe);
    kputc('\n');
}
