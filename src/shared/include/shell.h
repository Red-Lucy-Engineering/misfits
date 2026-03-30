#ifndef _SHL_H
#define _SHL_H

#include <stdint.h>

int shl_init();

/*
    The following functions are meant to be called as interrupts, they
    operate over the graphics table, which should be updated to send the
    data to the actual display.

    In the ESP32 backend, they are called by the actual interrupts that I
    suspect will be there, but for the SDL backend, they will be called
    by an OS-managed live loop.

    This file must have no hard dependencies on anything related to the
    actual core code for the ESP32.
*/

int shl_on_minute(); // Must be executed after display is ready, and after the timer interrupt.
int shl_on_button(uint8_t btn); // Must be executed on button interrupt (0-3)

#endif
