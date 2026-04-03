.equ INT_CONTROLLER_BASE , 0x10400
.equ INT_ENABLE          , 0x4
.equ INT_OUT             , 0x8

#include "display/lcd.s"
#include "input/buttons.s"
#include "serial/serial.s"
#include "timer/timer.s"
#include "spi/sd.s"
#include "spi/spi.s"


#include "./external_interrupts.s"
