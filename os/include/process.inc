PCB_SIZE EQU (TF_SIZE + )

STATE_UNUSED EQU 0
STATE_READY EQU 1
STATE_RUNNING EQU 2
STATE_BLOCKED EQU 3


;struct {
;    trap_frame_t tf,
;    u32 mepc,
;    u32 mstatus,
;    u32 mscratch, ; sp
;
;    u32 pid,
;    u32 pstatus,
;    void* next;
;
;    char* pname,
;    void* pentry,
;    u32 psize,
;
;} pcb_t;
