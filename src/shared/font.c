#include "include/font.h"
#include "include/filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint8_t  codepoint;
    uint16_t offset;
    uint8_t  vertex_count;
} __attribute__((packed)) fnt_glyph_entry_t;

static uint8_t           *memory      = NULL;
static fnt_glyph_entry_t *s_index     = NULL;
static uint16_t           glyph_count = 0;

int fnt_load(const char *path) {
    if (memory) {
        free(memory);
        memory = NULL;
        s_index = NULL;
        glyph_count = 0;
    }

    uint32_t size = 0;
    char *raw = fls_read(path, &size);
    if (!raw || size < 8) {
        printf("font.c: Could not open file, or is truncated! (%s)\n", path);
        free(raw);
        return -1;
    }

    if (memcmp(raw, "MSv01\0", 6) != 0) {
        printf("font.c: Invalid header! (%s)\n", path);
        free(raw);
        return -1;
    }

    uint16_t count = *(uint16_t *)(raw+6);

    uint32_t index_end = 8 + (uint32_t)count * 4;
    if (index_end > size) {
        printf("font.c: Failed sanity check! (%s)\n", path);
        free(raw);
        return -1;
    }

    memory      = (uint8_t *)raw;
    s_index     = (fnt_glyph_entry_t *)(memory + 8);
    glyph_count = count;

    return 0;
}

static const uint8_t *find_glyph(uint8_t cp) {
    int lo = 0, hi = (int)glyph_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const uint8_t *e = memory + 8 + mid * 4;
        if (e[0] == cp) return e;
        if (e[0]  < cp) lo = mid + 1;
        else            hi = mid - 1;
    }
    return NULL;
}

// returns how many pixels wide the glyph's ink is (based on max X vertex)
static uint16_t glyph_ink_w(uint8_t cp, uint16_t glyph_w) {
    const uint8_t *entry = find_glyph(cp);
    if (!entry) return glyph_w;

    uint16_t offset       = (uint16_t)(entry[1] | (entry[2] << 8));
    uint8_t  vertex_count = entry[3];
    const uint8_t *verts  = memory + offset;

    int max_x = 0;
    for (int i = 0; i < vertex_count; i++) {
        int x = (verts[i] >> 4) & 0xF;
        if (x > max_x) max_x = x;
    }
    return (uint16_t)(max_x * glyph_w / 15);
}

static void draw_glyph(uint8_t cp, uint16_t px, uint16_t py, uint16_t glyph_w, uint16_t glyph_h) {
    const uint8_t *entry = find_glyph(cp);

    if (!entry) {
        gra_rect_fill(1, px, py, glyph_w, glyph_h);
        return;
    }

    uint16_t offset       = (uint16_t)(entry[1] | (entry[2] << 8));
    uint8_t  vertex_count = entry[3];
    const uint8_t *verts  = memory + offset;
    if (vertex_count < 2)
        return;

    for (int i = 0; i < vertex_count - 1; i++) {
        uint8_t va = verts[i];
        uint8_t vb = verts[i + 1];

        int x0 = (va >> 4) & 0xF;
        int y0 = (va     ) & 0xF;
        int x1 = (vb >> 4) & 0xF;
        int y1 = (vb     ) & 0xF;

        int sx0 = px + (x0 * glyph_w / 16);
        int sy0 = py + (y0 * glyph_h / 16);
        int sx1 = px + (x1 * glyph_w / 16);
        int sy1 = py + (y1 * glyph_h / 16);

        gra_line(1, sx0, sy0, sx1, sy1);
    }
}

static uint16_t char_advance(uint8_t cp, uint16_t glyph_w, float spacing_factor) {
    uint16_t spacing = glyph_w * spacing_factor;
    if (cp == ' ')  return glyph_w + spacing;
    if (cp == '\t') return (glyph_w + spacing) * 4;
    return glyph_ink_w(cp, glyph_w) + spacing;
}

static int measure_line(const char *str, fnt_text_t cfg, uint16_t *out_px) {
    int count = 0;
    uint16_t x = 0;
    while (str[count] && str[count] != '\n') {
        uint16_t adv = char_advance((uint8_t)str[count], cfg.glyph_w, cfg.spacing_factor);
        if (x + adv > cfg.width) break;
        x += adv;
        count++;
    }
    if (out_px) *out_px = x;
    return count;
}

void fnt_print(const char *str, uint16_t x, uint16_t y, fnt_text_t cfg) {
    if (!str) return;

    uint16_t line_h = cfg.glyph_h;
    uint16_t cur_y  = y;
    const char *cursor = str;

    while (*cursor) {
        if (*cursor == '\n') {
            cur_y += line_h;
            cursor++;
            continue;
        }

        uint16_t line_px = 0;
        int line_len = measure_line(cursor, cfg, &line_px);
        if (line_len == 0) line_len = 1;

        uint16_t cur_x;
        switch (cfg.align) {
            case FNT_ALIGN_CENTER:
                cur_x = x + (cfg.width > line_px ? (cfg.width - line_px) / 2 : 0);
                break;
            case FNT_ALIGN_RIGHT:
                cur_x = x + (cfg.width > line_px ? cfg.width - line_px : 0);
                break;
            default:
                cur_x = x;
                break;
        }

        for (int i = 0; i < line_len; i++) {
            uint8_t cp = (uint8_t)cursor[i];
            if (cp != ' ' && cp != '\t')
                draw_glyph(cp, cur_x, cur_y, cfg.glyph_w, cfg.glyph_h);
            cur_x += char_advance(cp, cfg.glyph_w, cfg.spacing_factor);
        }

        cursor += line_len;
        cur_y  += line_h;
    }
}
