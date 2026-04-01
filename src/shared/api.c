#include "include/api.h"
#include "include/fe.h"
#include "include/graphics.h"
#include "include/font.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static char buf[1024];

static fe_Number _fe_numarg(fe_Context *ctx, fe_Object **args) {
    return fe_tonumber(ctx, fe_nextarg(ctx, args));
}

static fe_Object *api_mod(fe_Context *ctx, fe_Object *args) {
    fe_Number a = _fe_numarg(ctx, &args);
    fe_Number b = _fe_numarg(ctx, &args);
    return fe_number(ctx, fmodf(a, b));
}

static fe_Object *api_pow(fe_Context *ctx, fe_Object *args) {
    fe_Number base = _fe_numarg(ctx, &args);
    fe_Number exp  = _fe_numarg(ctx, &args);
    return fe_number(ctx, powf(base, exp));
}

static fe_Object *api_sqrt(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, sqrtf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_abs(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, fabsf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_floor(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, floorf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_ceil(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, ceilf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_min(fe_Context *ctx, fe_Object *args) {
    fe_Number r = _fe_numarg(ctx, &args);
    while (!fe_isnil(ctx, args)) {
        fe_Number v = _fe_numarg(ctx, &args);
        if (v < r) r = v;
    }
    return fe_number(ctx, r);
}

static fe_Object *api_max(fe_Context *ctx, fe_Object *args) {
    fe_Number r = _fe_numarg(ctx, &args);
    while (!fe_isnil(ctx, args)) {
        fe_Number v = _fe_numarg(ctx, &args);
        if (v > r) r = v;
    }
    return fe_number(ctx, r);
}

static fe_Object *api_clamp(fe_Context *ctx, fe_Object *args) {
    fe_Number v   = _fe_numarg(ctx, &args);
    fe_Number lo  = _fe_numarg(ctx, &args);
    fe_Number hi  = _fe_numarg(ctx, &args);
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return fe_number(ctx, v);
}

static fe_Object *api_lerp(fe_Context *ctx, fe_Object *args) {
    fe_Number a = _fe_numarg(ctx, &args);
    fe_Number b = _fe_numarg(ctx, &args);
    fe_Number t = _fe_numarg(ctx, &args);
    return fe_number(ctx, a + (b - a) * t);
}

static fe_Object *api_sin(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, sinf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_cos(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, cosf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_tan(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, tanf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_atan2(fe_Context *ctx, fe_Object *args) {
    fe_Number y = _fe_numarg(ctx, &args);
    fe_Number x = _fe_numarg(ctx, &args);
    return fe_number(ctx, atan2f(y, x));
}

static fe_Object *api_deg2rad(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, _fe_numarg(ctx, &args) * (M_PI / 180.0f));
}

static fe_Object *api_rad2deg(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, _fe_numarg(ctx, &args) * (180.0f / M_PI));
}

static fe_Object *api_log(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, logf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_log2(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, log2f(_fe_numarg(ctx, &args)));
}

static fe_Object *api_log10(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, log10f(_fe_numarg(ctx, &args)));
}

static fe_Object *api_exp(fe_Context *ctx, fe_Object *args) {
    return fe_number(ctx, expf(_fe_numarg(ctx, &args)));
}

static fe_Object *api_gt(fe_Context *ctx, fe_Object *args) {
    fe_Number a = _fe_numarg(ctx, &args);
    fe_Number b = _fe_numarg(ctx, &args);
    return fe_bool(ctx, a > b);
}

static fe_Object *api_gte(fe_Context *ctx, fe_Object *args) {
    fe_Number a = _fe_numarg(ctx, &args);
    fe_Number b = _fe_numarg(ctx, &args);
    return fe_bool(ctx, a >= b);
}

// TODO: replace with some better algo
static fe_Object *api_rand(fe_Context *ctx, fe_Object *args) {
    fe_Number r = (fe_Number)rand() / ((fe_Number)RAND_MAX + 1.0f);
    if (fe_isnil(ctx, args)) return fe_number(ctx, r);
    fe_Number a = _fe_numarg(ctx, &args);
    if (fe_isnil(ctx, args)) return fe_number(ctx, r * a);
    fe_Number b = _fe_numarg(ctx, &args);
    return fe_number(ctx, a + r * (b - a));
}

static fe_Object *api_strlen(fe_Context *ctx, fe_Object *args) {
    fe_tostring(ctx, fe_nextarg(ctx, &args), buf, sizeof(buf));
    return fe_number(ctx, (fe_Number)strlen(buf));
}

// TODO: gauge whether this even makes sense.
static fe_Object *api_strcat(fe_Context *ctx, fe_Object *args) {
    char result[2048] = {0};
    char part[512];
    while (!fe_isnil(ctx, args)) {
        fe_tostring(ctx, fe_nextarg(ctx, &args), part, sizeof(part));
        strncat(result, part, sizeof(result) - strlen(result) - 1);
    }
    return fe_string(ctx, result);
}

static fe_Object *api_numtostr(fe_Context *ctx, fe_Object *args) {
    fe_Number n = _fe_numarg(ctx, &args);
    if (n == (int)n)
        snprintf(buf, sizeof(buf), "%d", (int)n);
    else
        snprintf(buf, sizeof(buf), "%g", n);
    return fe_string(ctx, buf);
}

static fe_Object *api_strtonum(fe_Context *ctx, fe_Object *args) {
    fe_tostring(ctx, fe_nextarg(ctx, &args), buf, sizeof(buf));
    return fe_number(ctx, (fe_Number)atof(buf));
}

static fe_Object *api_time(fe_Context *ctx, fe_Object *args) {
    (void)args;
    return fe_number(ctx, (fe_Number)time(NULL));
}

static fe_Object *api_clear(fe_Context *ctx, fe_Object *args) {
    char b = !fe_isnil(ctx, fe_nextarg(ctx, &args));
    gra_clear(b);
    return fe_bool(ctx, 1);
}

static fe_Object *api_line(fe_Context *ctx, fe_Object *args) {
    char b = !fe_isnil(ctx, fe_nextarg(ctx, &args));
    fe_Number x0 = _fe_numarg(ctx, &args);
    fe_Number y0 = _fe_numarg(ctx, &args);
    fe_Number x1 = _fe_numarg(ctx, &args);
    fe_Number y1 = _fe_numarg(ctx, &args);
    gra_line(b, x0, y0, x1, y1);
    return fe_bool(ctx, 1);
}

static fe_Object *api_blit(fe_Context *ctx, fe_Object *args) {
    char b = !fe_isnil(ctx, fe_nextarg(ctx, &args));
    fe_Number x = _fe_numarg(ctx, &args);
    fe_Number y = _fe_numarg(ctx, &args);
    gra_blit(b, x, y);
    return fe_bool(ctx, 1);
}

static fe_Object *api_rect_line(fe_Context *ctx, fe_Object *args) {
    char b = !fe_isnil(ctx, fe_nextarg(ctx, &args));
    fe_Number x = _fe_numarg(ctx, &args);
    fe_Number y = _fe_numarg(ctx, &args);
    fe_Number w = _fe_numarg(ctx, &args);
    fe_Number h = _fe_numarg(ctx, &args);
    gra_rect_line(b, x, y, w, h);
    return fe_bool(ctx, 1);
}

static fe_Object *api_rect_fill(fe_Context *ctx, fe_Object *args) {
    char b = !fe_isnil(ctx, fe_nextarg(ctx, &args));
    fe_Number x = _fe_numarg(ctx, &args);
    fe_Number y = _fe_numarg(ctx, &args);
    fe_Number w = _fe_numarg(ctx, &args);
    fe_Number h = _fe_numarg(ctx, &args);
    gra_rect_fill(b, x, y, w, h);
    return fe_bool(ctx, 1);
}

static fe_Object *api_set_font(fe_Context *ctx, fe_Object *args) {
    fe_tostring(ctx, fe_nextarg(ctx, &args), buf, sizeof(buf));
    fnt_load(buf);
    return fe_bool(ctx, 1);
}

static fe_Object *api_write(fe_Context *ctx, fe_Object *args) {
    fe_tostring(ctx, fe_nextarg(ctx, &args), buf, sizeof(buf));
    fe_Number x = _fe_numarg(ctx, &args);
    fe_Number y = _fe_numarg(ctx, &args);

    fnt_text_t cfg = FNT_TEXT_DEFAULT;

    if (!fe_isnil(ctx, args)) cfg.glyph_w        = (uint16_t)_fe_numarg(ctx, &args);
    if (!fe_isnil(ctx, args)) cfg.glyph_h        = (uint16_t)_fe_numarg(ctx, &args);
    if (!fe_isnil(ctx, args)) cfg.align          = (fnt_align_t)(int)_fe_numarg(ctx, &args);
    if (!fe_isnil(ctx, args)) cfg.width          = (uint16_t)_fe_numarg(ctx, &args);
    if (!fe_isnil(ctx, args)) cfg.spacing_factor = (float)_fe_numarg(ctx, &args);

    fnt_print(buf, (uint16_t)x, (uint16_t)y, cfg);
    return fe_bool(ctx, 1);
}

typedef struct { const char *name; fe_CFunc fn; } fe_Registry;
typedef struct { const char *name; fe_Object *o; } fe_ValueRegistry;

static const fe_Registry api_entries[] = {
    { "mod",            api_mod      },
    { "pow",            api_pow      },
    { "sqrt",           api_sqrt     },
    { "abs",            api_abs      },
    { "floor",          api_floor    },
    { "ceil",           api_ceil     },
    { "min",            api_min      },
    { "max",            api_max      },
    { "clamp",          api_clamp    },
    { "lerp",           api_lerp     },
    { "sin",            api_sin      },
    { "cos",            api_cos      },
    { "tan",            api_tan      },
    { "atan2",          api_atan2    },
    { "deg2rad",        api_deg2rad  },
    { "rad2deg",        api_rad2deg  },
    { "log",            api_log      },
    { "log2",           api_log2     },
    { "log10",          api_log10    },
    { "exp",            api_exp      },
    { ">",              api_gt       },
    { ">=",             api_gte      },
    { "rand",           api_rand     },
    { "strlen",         api_strlen   }, // TODO: Consider generalizing and renaming to # ?
    { "cat",            api_strcat   },
    { "number2string",  api_numtostr }, // TODO: TOSTRING, TONUMBER.
    { "string2number",  api_strtonum },
    { "time",           api_time     },

    { "clear",          api_clear     },
    { "line",           api_line      },
    { "blit",           api_blit      },
    { "rect_line",      api_rect_line },
    { "rect_fill",      api_rect_fill },
    { "set_font",       api_set_font  },
    { "write",          api_write     },

    { NULL, NULL }
};

void api_register(fe_Context *ctx) {
    fe_set(ctx, fe_symbol(ctx, "SCR_WIDTH"),  fe_number(ctx, GRA_SCREEN_WIDTH));
    fe_set(ctx, fe_symbol(ctx, "SCR_HEIGHT"), fe_number(ctx, GRA_SCREEN_HEIGHT));
    fe_set(ctx, fe_symbol(ctx, "PI"),         fe_number(ctx, M_PI));

    fe_set(ctx, fe_symbol(ctx, "true"),  fe_bool(ctx, 1));
    fe_set(ctx, fe_symbol(ctx, "false"), fe_bool(ctx, 0));

    fe_set(ctx, fe_symbol(ctx, "ALIGN_LEFT"), fe_number(ctx, FNT_ALIGN_LEFT));
    fe_set(ctx, fe_symbol(ctx, "ALIGN_RIGHT"), fe_number(ctx, FNT_ALIGN_RIGHT));
    fe_set(ctx, fe_symbol(ctx, "ALIGN_CENTER"), fe_number(ctx, FNT_ALIGN_CENTER));

    for (const fe_Registry *e = api_entries; e->name; e++)
        fe_set(ctx, fe_symbol(ctx, e->name), fe_cfunc(ctx, e->fn));
}
