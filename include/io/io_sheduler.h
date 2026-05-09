#pragma once
#include <types.h>
// A single pending read request submitted by the FSM layer.
// The FSM allocates this (or uses a pool), fills it, and submits.
// On completion the scheduler calls callback(ctx, status) then frees.

typedef void (*io_complete_cb)(void* ctx, int status);

typedef struct io_sched_req {
    uint32_t           lba;
    io_complete_cb     callback;
    void*              ctx;
    struct io_sched_req* next;
} io_sched_req_t;

typedef enum {
    SCHED_IDLE,
    SCHED_BUSY,
} sched_state_t;

typedef struct {
    io_sched_req_t* queue_head;   // pending requests (FIFO)
    io_sched_req_t* queue_tail;
    io_sched_req_t* in_flight;    // currently issued to hardware, or NULL
    sched_state_t   state;
} io_scheduler_t;

extern io_scheduler_t g_io_sched;

void io_sched_submit(io_sched_req_t* req);
