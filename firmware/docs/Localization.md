# misfits Firmware Documentation - Localization

This document describes the internationalization and localization architecture
implemented in [`main/localisation.h`](../main/localisation.h) and
[`main/firmware.c`](../main/firmware.c).

## Overview

The misfits watch face presents localized date information (day of the week and
month). Rather than including heavy localization runtimes, the firmware uses
lightweight, compile-time string tables matched directly to C standard library
`struct tm` indexing.

## Language Selection Macros

Language selection is controlled at compile time using preprocessor macros:

| Macro | Language | Sample Output |
| :--- | :--- | :--- |
| `CFG_LANG_DE` / `FG_LANG_DE` | German | `"Di, 29. September"` |
| `CFG_LANG_EN` (or default fallback) | English | `"Tue, 29. September"` |

### Implementation (`main/localisation.h`)

```c
#pragma once

#if defined(CFG_LANG_EN)
static const char * const weekday_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char * const month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
#elif defined(CFG_LANG_DE) || defined(FG_LANG_DE)
static const char * const weekday_names[] = {
    "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"
};

static const char * const month_names[] = {
    "Januar", "Februar", "März", "April", "Mai", "Juni",
    "Juli", "August", "September", "Oktober", "November", "Dezember"
};
#else
static const char * const weekday_names[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char * const month_names[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
#endif
```

## Array Index Mapping

The string table indices map directly to standard C library `struct tm`
fields without requiring offset arithmetic:

- **Weekday Array (`weekday_names[now->tm_wday]`)**:
  - `0`: Sunday (`"Sun"` / `"So"`)
  - `1`: Monday (`"Mon"` / `"Mo"`)
  - `2`: Tuesday (`"Tue"` / `"Di"`)
  - `3`: Wednesday (`"Wed"` / `"Mi"`)
  - `4`: Thursday (`"Thu"` / `"Do"`)
  - `5`: Friday (`"Fri"` / `"Fr"`)
  - `6`: Saturday (`"Sat"` / `"Sa"`)

- **Month Array (`month_names[now->tm_mon]`)**:
  - `0`: January (`"January"` / `"Januar"`)
  - ...
  - `11`: December (`"December"` / `"Dezember"`)

## UTF-8 Compatibility

Localized languages often require multi-byte characters (e.g. `"März"` contains
the German umlaut `ä`, encoded as UTF-8 bytes `0xC3 0xA4`). 

Because the typography subsystem ([`main/glyph.c`](../main/glyph.c)) features a
full UTF-8 streaming parser, multi-byte localized characters are correctly parsed
into Unicode codepoints (such as `0x00E4` for `ä`) and matched in the
pre-compiled [`freemono_small`](../main/freemono_fonts.h) glyph table without
string corruption or truncation.

## Timezone and Daylight Saving Time (DST)

System timezone rules are configured in [`main/firmware.c`](../main/firmware.c)
using the standard POSIX timezone environment variable:

```c
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
```

### Syntax Breakdown

- `CET-1`: Standard time zone abbreviation **CET** (Central European Time) with
  a UTC offset of +1 hour. (Note: in POSIX syntax, the sign is reversed relative
  to UTC offsets).
- `CEST`: Daylight saving time abbreviation **CEST** (Central European Summer
  Time), representing a UTC offset of +2 hours.
- `M3.5.0`: DST start rule:
  - `M3`: Month 3 (March)
  - `.5`: Week 5 (the last occurrence of the day in the month)
  - `.0`: Day 0 (Sunday)
  - Default transition time: 02:00:00 local time.
- `M10.5.0/3`: DST end rule:
  - `M10`: Month 10 (October)
  - `.5`: Week 5 (the last occurrence of the day in the month)
  - `.0`: Day 0 (Sunday)
  - `/3`: Transition occurs at 03:00:00 local time.

When the device receives a UNIX timestamp via BLE, the C standard library
function `localtime_r()` automatically applies the rules defined in `TIMEZONE` to
derive the correct local hour, minute, and DST state.

## Configuring Language at Build Time

To select German at compile time, add the compilation definition in
[`main/CMakeLists.txt`](../main/CMakeLists.txt):

```cmake
target_compile_definitions(${COMPONENT_LIB} PRIVATE CFG_LANG_DE)
```

Alternatively, pass it via the command line when invoking the build:

```bash
idf.py build -DCOMPILE_DEFINITIONS="CFG_LANG_DE"
```
