#ifndef PROG_H
#define PROG_H

#include "memplus.h"
#include <stdbool.h>

#define DB_FILENAME "crt.db"

typedef enum {
    SHELL_FISH
} Shell;

typedef struct {
    mp_Allocator *alloc;
    const char   *home_dir;
    mp_String     xdg_data_home;
    mp_String     xdg_cache_home;

    Shell shell;
    bool  verbose;
    bool  test;
} Prog;

#endif /* ifndef PROG_H */
