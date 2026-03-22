timer_interrupt:
    li      t0, 400000               ; Interval = 10 ms in ticks

    lw      t1, 0x718                ; Load mtimecmp low
    lw      t2, 0x71C                ; Load mtimecmp high

    add     t3, t1, t0               ; low = mtimecmp_lo + interval
    sltu    t4, t3, t0               ; carry if overflow
    add     t2, t2, t4               ; high += carry

    sw      t3, 0x718                ; Write mtimecmp low
    sw      t2, 0x71C                ; Write mtimecmp high
		ret
