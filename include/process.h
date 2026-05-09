#pragma once
#include "trap.h"

#define STATE_UNUSED 0
#define STATE_READY 1
#define STATE_RUNNING 2
#define STATE_BLOCKED 3

#define STACK_SIZE 8192

typedef struct {
    trap_frame_t tf;
    int mepc;
    int mstatus;
    int mscratch;


    void* next;

    char* pname;

    void* brk; // current process brk

    void* pentry; // this is text section base offset
    int ptext_size; // this is used as text section limit
    int parent_dir_cluster;// this is the lba of the parent directory start?

    void* dmmu_physical; // this is the physical base address
    void* pdata_start; // this is the .rodata section and is used for data MMU virt start
    void* heap_start;  // pdata_start -> heap_start = rodata + data + bss 
                       // this is used to calcualte dmmu limit as (dmmu_physical+heap_start-pdata_start+STACK_SIZE)
    
    // when forking allocate  (heap_start-pdata_start)+STACK_SIZE 
    // copy pdata_start - pdata_start+allocate_size
    // copy pentry and ptext_size (this is shared region)


    unsigned char pid;
    unsigned char ppid; // parent id
    char exit_code; // for parent
    unsigned char pstatus;
    unsigned char wait_reason; // track if it is waiting on SERIAL, FILE IO, ect

} pcb_t;
