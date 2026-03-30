#include "../shared/include/filesystem.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Super basic implementation, we don't need much more than this.

int fls_init() {
    const char *path = getenv("MAWS_PATH");
    if (path == NULL) {
        printf("MAWS: Must specify MAWS_PATH!\n");
        return 1;
    }

    if (chdir(path)) {
        printf("MAWS: Could not C to '%s' (MAWS_PATH)!\n", path);
        return 1;
    }

    return 0;
}

char *fls_read(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0, SEEK_END);
    uint32_t s = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Alloc an extra byte JUST IN CASE!
    char *memory = malloc(s+1);
    memory[s] = 0;

    fread(memory, s, 1, f);
    if (size != NULL)
        *size = s;

    return memory;
}
