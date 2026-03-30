#include "include/shell.h"
#include "include/api.h"
#include "include/filesystem.h"
#include "include/fe.h"
#include "include/graphics.h"

#define FE_MEMORY_SIZE (1024 * 1024 * 8)
void *fe_memory = NULL;
fe_Context *fe_context = NULL;
char *fe_program = NULL;


static char fe_read_memory(fe_Context *ctx, void *udata) {
    static uint32_t counter = 0;
    return fe_program[counter++];
}

static void fe_callback(const char *name, unsigned int number) {
    int gc = fe_savegc(fe_context);

    fe_Object *objs[2];
    objs[0] = fe_symbol(fe_context, name);
    objs[1] = fe_number(fe_context, number);

    fe_eval(fe_context, fe_list(fe_context, objs, 1));
    fe_restoregc(fe_context, gc);
}

typedef int esp_err_t;

int shl_init() {
    fe_program = fls_read("program.fe", NULL);
    if (fe_program == NULL) {
        printf("Oops! No program.\n");
        return 1;
    }

    fe_memory = malloc(FE_MEMORY_SIZE);
    if (fe_memory == NULL) {
        printf("Could not allocate memory!\n");
        return 1;
    }

    fe_context = fe_open(fe_memory, FE_MEMORY_SIZE);

    int gc = fe_savegc(fe_context);
    api_register(fe_context);
    while (1) {
        fe_Object *obj = fe_read(fe_context, fe_read_memory, NULL);
        if (!obj) break;
        fe_eval(fe_context, obj);
        fe_restoregc(fe_context, gc);
    }

    fe_callback("on_init", 0);

    return 0;
}

int shl_on_minute() {
    fe_callback("on_minute", 0);
    return 0;
}

int shl_on_button(uint8_t btn) {
    fe_callback("on_button", btn);
    return 0;
}
