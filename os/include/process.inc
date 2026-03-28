#include "trap.inc"

STATE_UNUSED EQU 0
STATE_READY EQU 1
STATE_RUNNING EQU 2
STATE_BLOCKED EQU 3

STACK_SIZE EQU 4096 ; Bytes (This is also the theoretical limit of sbrk)


STRUCT
RECORD TRAP_FRAME_SIZE
PCB_MEPC WORD
PCB_MSTATUS WORD
PCB_MSCRATCH WORD

PCB_PID WORD
PCB_STATUS BYTE
PCB_NEXT WORD

PCB_PNAME WORD
PCB_PENTRY WORD
PCB_PSIZE WORD

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
