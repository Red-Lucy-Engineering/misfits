#ifndef _GRA_H
#define _GRA_H

#include <stdint.h>

/*
    Buffer size is divided by 8 since the buffer itself is tightly packed,
    each pixel is exactly one bit, either on or off.

    I decided to use 16 bit integers for the coordinates as subsequent
    versions of the Dope Ass Misfits Technology(tm) might have bigger
    screens.

    This file must have no hard dependencies on anything related to the
    actual core code for the ESP32.

    I use chars as booleans.
*/

#define GRA_SCREEN_WIDTH 200
#define GRA_SCREEN_HEIGHT 200
#define GRA_BUFFER_SIZE ((GRA_SCREEN_WIDTH * GRA_SCREEN_HEIGHT) >> 3) // (/ 8)

extern uint8_t gra_screen_buffer[];

void gra_clear(char black);
void gra_blit(char black, uint16_t x, uint16_t y);
void gra_rect_fill(char black, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void gra_rect_line(char black, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void gra_line(char black, int x0, int y0, int x1, int y1);

#endif
