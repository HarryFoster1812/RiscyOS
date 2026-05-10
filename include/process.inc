#pragma once
#include <trap.inc>

STATE_UNUSED EQU 0
STATE_READY EQU 1
STATE_RUNNING EQU 2
STATE_BLOCKED EQU 3

STACK_SIZE EQU 8192 ; Bytes (This is also the theoretical limit of sbrk)


STRUCT
RECORD TRAP_FRAME_SIZE
PCB_MEPC WORD
PCB_MSTATUS WORD
PCB_MSCRATCH WORD

PCB_NEXT WORD

PCB_PNAME WORD
PCB_BRK WORD

PCB_PTEXT_MEMORY_REGION WORD
PCB_PDATA_MEMORY_REGION WORD

PCB_PARENT_DIR_CLUSTER WORD

PCB_PDATA_START WORD
PCB_PHEAP_START WORD ; needed to prevent sbrk(-n)
PCB_FORK_MEM_CPY_SIZE ALIAS
PCB_PID BYTE
PCB_PPID BYTE
PCB_EXIT_CODE BYTE
PCB_STATUS BYTE
PCB_WAIT_REASON BYTE

STRUCT_ALIGN
PCB_SIZE ALIAS

;struct {
;    trap_frame_t tf,
;    u32 mepc,
;    u32 mstatus,
;    u32 mscratch, ; sp
;
;    u32 pid,
;    u32 pstatus,
;    void* next,
;
;    char* pname,
;    void* pentry,
;    u32 psize,
;
;} pcb_t;
