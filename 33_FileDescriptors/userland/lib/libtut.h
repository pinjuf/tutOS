#pragma once

#include "unistd.h"

#define PIT0_FREQ 2000

extern int stdout, * stdin;

void _entry();
int main(int argc, char * argv[]);

void pit_msleep(size_t ms);
