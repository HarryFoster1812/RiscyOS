#pragma once
#include "trap.h"

#define STATE_UNUSED 0
#define STATE_READY 1
#define STATE_RUNNING 2
#define STATE_BLOCKED 3

#define STACK_SIZE 4096 

typedef struct {
    trap_frame_t tf;
    int mepc;
    int mstatus;
    int mscratch;

    int pid;
    int pstatus;
    void* next;

    char* pname;
    void* pentry;
    int psize;

} pcb_t;
