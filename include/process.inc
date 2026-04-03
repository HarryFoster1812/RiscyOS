#pragma once
#include "./struct_macros.inc"
#include "trap.inc"

.equ STATE_UNUSED, 0
.equ STATE_READY, 1
.equ STATE_RUNNING, 2
.equ STATE_BLOCKED, 3

.equ STACK_SIZE, 4096 


STRUCT_BEGIN
RECORD(TRAP_FRAME_SIZE)
WORD(PCB_MEPC)
WORD(PCB_MSTATUS)
WORD(PCB_MSCRATCH)

BYTE(PCB_PID)
BYTE(PCB_STATUS)
WORD(PCB_NEXT)

WORD(PCB_PNAME)
WORD(PCB_PENTRY)
WORD(PCB_PSIZE)

STRCT_END(PCB_SIZE)


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
