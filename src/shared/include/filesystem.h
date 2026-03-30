#ifndef _FLS_H
#define _FLS_H

#include <stdint.h>
#include <stddef.h>

int fls_init();

// You own the memory that comes out of this, must be freeable by free()
// size may be NULL
char *fls_read(const char *path, uint32_t *size);

#endif
