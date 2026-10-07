#include "glyph.h"
#include "eink.h"

const bitmap_glyph_t *find_glyph(
    const bitmap_font_t *font, uint32_t codepoint) {
    // table was sorted by converter by unicode-codepoint
    size_t lo = 0;
    size_t hi = font->count;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        uint32_t current = font->glyphs[mid].codepoint;

        if (current < codepoint) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo < font->count && font->glyphs[lo].codepoint == codepoint) {
        return &font->glyphs[lo];
    }
    return NULL;
}

// read in utf8, so umlauts and similar can be displayed.
// Invalid sequences will be skipped
uint32_t next_utf8(const char **text) {
    const unsigned char *p = (const unsigned char *)*text;
    uint32_t cp;

    if (p[0] < 0x80) {
        cp = p[0];
        *text += 1;
    } else if ((p[0] & 0xE0) == 0xC0 && p[1] &&
               (p[1] & 0xC0) == 0x80) {
        cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *text += 2;
    } else if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2] &&
               (p[1] & 0xC0) == 0x80 &&
               (p[2] & 0xC0) == 0x80) {
        cp = ((uint32_t)(p[0] & 0x0F) << 12) |
             ((uint32_t)(p[1] & 0x3F) << 6) |
             (p[2] & 0x3F);
        *text += 3;
    } else if ((p[0] & 0xF8) == 0xF0 && p[1] && p[2] && p[3] &&
               (p[1] & 0xC0) == 0x80 &&
               (p[2] & 0xC0) == 0x80 &&
               (p[3] & 0xC0) == 0x80) {
        cp = ((uint32_t)(p[0] & 0x07) << 18) |
             ((uint32_t)(p[1] & 0x3F) << 12) |
             ((uint32_t)(p[2] & 0x3F) << 6) |
             (p[3] & 0x3F);
        *text += 4;
    } else {
        cp = '?';
        *text += 1;
    }

    return cp;
}

int text_width(const bitmap_font_t *font, const char *text)
{
    int width = 0;

    while (*text) {
        const bitmap_glyph_t *g = find_glyph(font, next_utf8(&text));
        if (g) {
            width += g->advance;
        }
    }
    return width;
}

// x: Text begin. baseline_y: Font baseline.
void draw_text(const bitmap_font_t *font,
               int x, int baseline_y, const char *text) {
    while (*text) {
        const bitmap_glyph_t *g = find_glyph(font, next_utf8(&text));
        if (!g) {
            continue; // Symbol not found in this size
        }

        int row_bytes = (g->width + 7) / 8;

        for (int gy = 0; gy < g->height; gy++) {
            for (int gx = 0; gx < g->width; gx++) {
                uint8_t mask = 0x80 >> (gx % 8);

                // in glyph array: set bit = black Pixel.
                if (!(g->bitmap[gy * row_bytes + gx / 8] & mask)) {
                    continue;
                }

                int px = x + g->x_offset + gx;
                int py = baseline_y + g->y_offset + gy;

                if (px >= 0 && px < DISPLAY_W &&
                    py >= 0 && py < DISPLAY_H) {
                    // in framebuffer: deleted Bit = black.
                    framebuffer[py * (DISPLAY_W / 8) + px / 8] &=
                        (uint8_t)~(0x80 >> (px % 8));
                }
            }
        }

        x += g->advance;
    }
}
