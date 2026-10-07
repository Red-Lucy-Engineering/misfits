#ifndef _GLYPH_H
#define _GLYPH_H

#include <stddef.h>
#include <stdint.h>
#include "freemono_fonts.h"

const bitmap_glyph_t *find_glyph(const bitmap_font_t *font, uint32_t codepoint);

uint32_t next_utf8(const char **text);

int text_width(const bitmap_font_t *font, const char *text);

void draw_text(const bitmap_font_t *font, int x, int baseline_y, const char *text);

#endif //_GLYPH_H
