#include <io/sd_state.h>
#include <io/sd_functions.h>
#include <io/io_sd_ring.h>
extern SD_INFO_T SD_INFO;

void sd_irsq_handler(void) {
    // Tail action depends only on operation direction — no FSM state.
    switch ((SD_STATE_T)SD_INFO.SD_STATE) {
        case SD_STATE_WAIT_READ:  sd_tail_read();  break;
        case SD_STATE_WAIT_WRITE: sd_tail_write(); break;
        default: break; // spurious IRQ, ignore
    }

    uint8_t idx = g_sd_irq_ring.head & (IRQ_RING_SIZE - 1);
    g_sd_irq_ring.status[idx] = 0;
    g_sd_irq_ring.head++;
}
