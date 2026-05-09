#include <io/io_sheduler.h>
#include <io/sd_functions.h>
#include <io/io_sd_ring.h>
#include <mm.h>

static void io_sched_issue_next(void) {
    if (!g_io_sched.queue_head) return;

    io_sched_req_t* req   = g_io_sched.queue_head;
    g_io_sched.queue_head = req->next;
    if (!g_io_sched.queue_head)
        g_io_sched.queue_tail = NULL;

    g_io_sched.in_flight = req;
    g_io_sched.state     = SCHED_BUSY;
    sd_start_read(req->lba); 
}
// Called by FSM layer only. Safe to call from callback context.
void io_sched_submit(io_sched_req_t* req) {
    req->next = NULL;

    // Enqueue at tail
    if (g_io_sched.queue_tail)
        g_io_sched.queue_tail->next = req;
    else
        g_io_sched.queue_head = req;
    g_io_sched.queue_tail = req;

    // If idle, issue immediately
    if (g_io_sched.state == SCHED_IDLE)
        io_sched_issue_next();
    // else: queued, will be issued by pump after current completes
}


void io_sched_pump(void) {
    while (g_sd_irq_ring.head != g_sd_irq_ring.tail) {
        uint8_t idx    = g_sd_irq_ring.tail & (IRQ_RING_SIZE - 1);
        int     status = (int)g_sd_irq_ring.status[idx];
        g_sd_irq_ring.tail++;

        io_sched_req_t* req  = g_io_sched.in_flight;
        g_io_sched.in_flight = NULL;
        g_io_sched.state     = SCHED_IDLE;

        // Resume FSM callback may call io_sched_submit() but NOT io_sched_pump()
        req->callback(req->ctx, status);
        kfree(req);

        // Issue next if the FSM (or anyone else) queued something
        if (g_io_sched.state == SCHED_IDLE && g_io_sched.queue_head)
            io_sched_issue_next();
    }
}
