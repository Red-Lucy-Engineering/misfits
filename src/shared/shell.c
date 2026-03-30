// TODO: Port to the ESP error handling type thingy.

#include "include/shell.h"
#include "include/graphics.h"
#include "include/filesystem.h"
#include "include/fe.h"

int shl_init() {
    const char *program = fls_read("program.fe", NULL);
    if (program == NULL) {
        printf("Oops! No program.\n");
        return 1;
    }

    return 0;
}

int shl_on_minute() {
    gra_clear(0);
    gra_rect_line(1, 5, 5, GRA_SCREEN_WIDTH-10, GRA_SCREEN_HEIGHT-10);

    // TODO: Finish lol

    return 0;
}

int shl_on_button(uint8_t btn) {
    gra_clear(0);
    return 0;
}
