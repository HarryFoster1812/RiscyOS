#pragma once
#include <types.h>
#define IRQ_RING_SIZE 8   // must be power of 2

typedef struct {
    volatile uint8_t head;   // written by IRQ handler
    volatile uint8_t tail;   // read  by io_sched_pump()
    volatile uint8_t status[IRQ_RING_SIZE]; // 0 = OK, nonzero = error
} irq_completion_ring_t;

extern irq_completion_ring_t g_sd_irq_ring;

