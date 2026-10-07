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
