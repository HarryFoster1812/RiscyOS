INT_CONTROLLER_BASE EQU 0x10400
INT_ENABLE          EQU 0x4
INT_OUT             EQU 0x8

#include "display/lcd.s"
#include "input/buttons.s"
#include "serial/serial.s"
#include "timer/timer.s"
#include "spi/sd.s"
#include "spi/spi.s"


#include "./external_interrupts.s"
