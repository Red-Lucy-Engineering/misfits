#ifndef _FNT_H
#define _FNT_H

#include <stdint.h>
#include "graphics.h"

typedef enum {
    FNT_ALIGN_LEFT,
    FNT_ALIGN_CENTER,
    FNT_ALIGN_RIGHT,
} fnt_align_t;

typedef struct {
    uint16_t glyph_w;
    uint16_t glyph_h;
    float    spacing_factor;
    uint16_t width;
    fnt_align_t align;
} fnt_text_t;

#define FNT_TEXT_DEFAULT ((fnt_text_t){ \
    .glyph_w        = 16,               \
    .glyph_h        = 16,               \
    .spacing_factor = 0.4f,             \
    .width          = GRA_SCREEN_WIDTH, \
    .align          = FNT_ALIGN_LEFT,   \
})

// returns 0 on success, -1 on failure.
int fnt_load(const char *path);

// draw a string at (x, y) using the given text config.
void fnt_print(const char *str, uint16_t x, uint16_t y, fnt_text_t cfg);

#endif
