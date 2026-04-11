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

    void* brk; // current process brk

    void* pentry; // this is text section base offset
    int ptext_size; // this is used as text section limit

    void* pdata_start; // this is the .rodata section and is used for data MMU offset
    void* heap_start;  // pdata_start -> heap_start = rodata + data + bss
    
    // when forking allocate  (heap_start-pdata_start)+STACK_SIZE 
    // copy pdata_start - pdata_start+allocate_size
    // copy pentry and ptext_size (this is shared region)

} pcb_t;
