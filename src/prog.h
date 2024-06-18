#ifndef PROG_H
#define PROG_H

#include "memplus.h"
#include <stdbool.h>

#define DB_DIR      "crt"
#define DB_FILENAME "crt.db"

typedef enum {
    SHELL_FISH,
    SHELL_ATUIN,
    SHELL_COUNT,
} Shell;

typedef struct {
    mp_Allocator *alloc;
    const char   *home_dir;
    mp_String     xdg_data_home;
    mp_String     data_home;

    Shell shell;
    bool  verbose;
    bool  update;
    bool  test;
} Prog;

#endif /* ifndef PROG_H */
