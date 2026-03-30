#include "include/graphics.h"
#include <string.h>

uint8_t gra_screen_buffer[GRA_BUFFER_SIZE];

#define APPLY_MASK(row, byte_idx, mask, black) \
    do { \
        if (black) (row)[byte_idx] |=  (mask); \
        else       (row)[byte_idx] &= ~(mask); \
    } while (0)

#define RECT_CLAMP(x, y, w, h) \
    do { \
        if ((w) == 0 || (h) == 0)                  return; \
        if ((x) >= GRA_SCREEN_WIDTH  || \
            (y) >= GRA_SCREEN_HEIGHT)               return; \
        if ((x) + (w) > GRA_SCREEN_WIDTH)  (w) = GRA_SCREEN_WIDTH  - (x); \
        if ((y) + (h) > GRA_SCREEN_HEIGHT) (h) = GRA_SCREEN_HEIGHT - (y); \
    } while (0)

#define abs_i(v) ((v) < 0 ? -(v) : (v))

/*
 * draw_hline() - draws one horizontal span into a pre-computed row pointer,
 * using byte-aligned writes for the interior and bitmasks for the edges!!
 */
static inline void draw_hline(char black, uint8_t *row,
                               int start_byte, int end_byte,
                               uint8_t left_mask, uint8_t right_mask,
                               uint8_t fill_byte) {
    if (start_byte == end_byte) {
        APPLY_MASK(row, start_byte, left_mask & right_mask, black);
        return;
    }

    APPLY_MASK(row, start_byte, left_mask,  black);
    APPLY_MASK(row, end_byte,   right_mask, black);

    if (end_byte > start_byte + 1)
        memset(&row[start_byte + 1], fill_byte, end_byte - start_byte - 1);
}

static inline void quick_blit(char black, uint16_t x, uint16_t y) {
    uint32_t idx  = (uint32_t)y * GRA_SCREEN_WIDTH + x;
    uint8_t *byte = &gra_screen_buffer[idx >> 3];
    uint8_t  mask = (uint8_t)(1u << (7 - (idx & 7)));

    if (black) *byte |=  mask;
    else       *byte &= ~mask;
}

/**
 * gra_clear() - fills the screen buffer with a single color.
 * @black: If non-zero, fills with black (0xFF); otherwise fills with white (0x00).
 */
void gra_clear(char black) {
    memset(gra_screen_buffer, black ? 0xFF : 0x00, GRA_BUFFER_SIZE);
}

void gra_blit(char black, uint16_t x, uint16_t y) {
    if (x >= GRA_SCREEN_WIDTH || y >= GRA_SCREEN_HEIGHT)
        return;

    quick_blit(black, x, y);
}

/*
 * gra_rect_fill() - fills a rectangle using byte-aligned writes where possible,
 * falling back to bitmasks only for the partial left and right edge bytes.
 */
void gra_rect_fill(char black, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    RECT_CLAMP(x, y, w, h);

    const int bytes_per_row = GRA_SCREEN_WIDTH >> 3;
    uint8_t fill_byte = black ? 0xFF : 0x00;

    uint16_t x1       = x + w;
    int start_byte    = x  >> 3;
    int end_byte      = (x1 - 1) >> 3;
    uint8_t left_mask  = (uint8_t)(0xFF >> (x & 7));
    uint8_t right_mask = (uint8_t)(0xFF << (7 - ((x1 - 1) & 7)));

    uint8_t *row = gra_screen_buffer + y * bytes_per_row;
    for (uint16_t i = 0; i < h; i++) {
        draw_hline(black, row, start_byte, end_byte, left_mask, right_mask, fill_byte);
        row += bytes_per_row;
    }
}

void gra_rect_line(char black, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    RECT_CLAMP(x, y, w, h);

    const int bytes_per_row = GRA_SCREEN_WIDTH >> 3;
    uint8_t fill_byte = black ? 0xFF : 0x00;

    uint16_t x1        = x + w;
    int start_byte     = x  >> 3;
    int end_byte       = (x1 - 1) >> 3;
    uint8_t left_mask  = (uint8_t)(0xFF >> (x & 7));
    uint8_t right_mask = (uint8_t)(0xFF << (7 - ((x1 - 1) & 7)));

    draw_hline(black, gra_screen_buffer + y * bytes_per_row,
               start_byte, end_byte, left_mask, right_mask, fill_byte);

    if (h > 1)
        draw_hline(black, gra_screen_buffer + (y + h - 1) * bytes_per_row,
                   start_byte, end_byte, left_mask, right_mask, fill_byte);

    if (h > 2) {
        int lbyte      = x >> 3;
        int rbyte      = (x + w - 1) >> 3;
        uint8_t lmask  = (uint8_t)(1u << (7 - (x & 7)));
        uint8_t rmask  = (uint8_t)(1u << (7 - ((x + w - 1) & 7)));

        for (uint16_t yy = y + 1; yy < y + h - 1; yy++) {
            uint8_t *row = gra_screen_buffer + yy * bytes_per_row;
            APPLY_MASK(row, lbyte, lmask, black);
            APPLY_MASK(row, rbyte, rmask, black);
        }
    }
}

// Bresenham's line thingy :)
void gra_line(char black, int x0, int y0, int x1, int y1) {
    int dx =  abs_i(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;

    int dy = -abs_i(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        gra_blit(black, x0, y0);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
