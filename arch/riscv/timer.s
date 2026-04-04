timer_interrupt:
    li      t0, TIMER_INTERVAL        ; interval

    li      t5, SYSTEM_BASE          ; mtime address (platform-specific)
    lw      t1, SYSTEM_LOW_MTIME[t5]               ; mtime low
    lw      t2, SYSTEM_HIGH_MTIME[t5]               ; mtime high

    add     t3, t1, t0
    sltu    t4, t3, t1              ; carry
    add     t2, t2, t4

    li      t4, -1
    sw      t4, SYSTEM_HIGH_MTIMECMP[t5]               ; set high to max

    sw      t3, SYSTEM_LOW_MTIMECMP[t5]               ; write low
    sw      t2, SYSTEM_HIGH_MTIMECMP[t5]               ; write high

		lw t0, system_ticks
		addi t0, t0, 1
		sw t0, system_ticks, t6
		
		; reduce sleep timer 

		andi t2, t0, TIME_SLICE_MAX-1
		bnez t2, %F1

		; run scheduler
		call schedule

		1
		; need to decide
    ret

TIMER_INTERVAL_MS EQU 5

TIMER_INTERVAL EQU (TIMER_INTERVAL_MS*1000000)/25


TIME_SLICE_MAX EQU 4 ; 20 ms max execution time

SYSTEM_BASE EQU 0x0001_0700
STRUCT
SYSTEM_VERSION WORD
SYSTEM_HALT WORD
SYSTEM_CLOCK_SPEED WORD
SYSTEM_IO_PIN WORD
SYSTEM_LED_SRC WORD
SYSTEM_LOW_MTIME WORD
SYSTEM_HIGH_MTIME WORD
SYSTEM_LOW_MTIMECMP WORD
SYSTEM_HIGH_MTIMECMP WORD

system_ticks: DEFW 0x0
