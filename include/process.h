#pragma once
#include "trap.h"
#include "types.h"

#define STATE_UNUSED 0
#define STATE_READY 1
#define STATE_RUNNING 2
#define STATE_BLOCKED 3

#define STACK_SIZE 8192

typedef struct {
	void* physical_base;
	unsigned int region_size;
	unsigned int reference_count;
} memory_region_t;

typedef struct {
    trap_frame_t tf;
    int mepc;
    int mstatus;
    int mscratch;


    void* next;

    char* pname;

    void* brk; // current process brk

    memory_region_t* ptext_memory_region; // this is text section base offset
    memory_region_t* pdata_memory_region; // this is text section base offset

    int parent_dir_cluster;// this is the cluster no of the parent directory 
    void* pdata_start; // this is the .rodata section and is used for data MMU virt start
    void* heap_start;  // pdata_start -> heap_start = rodata + data + bss 
    

    unsigned char pid;
    unsigned char ppid; // parent id
    char exit_code; // for parent
    unsigned char pstatus;
    unsigned char wait_reason; // track if it is waiting on SERIAL, FILE IO, ect

} pcb_t;



extern pcb_t* get_pcb_from_id(uint8_t pid);
extern void unblock_process(pcb_t* pcb);
