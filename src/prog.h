#ifndef PROG_H
#define PROG_H

#include "memplus.h"
#include <stdbool.h>
#include <time.h>

#define DB_DIR      "crt"
#define DB_FILENAME "crt.db"

typedef enum {
    SHELL_FISH,
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

typedef struct {
    time_t first_checked[SHELL_COUNT];
    int    count[SHELL_COUNT];
} Db;

#endif /* ifndef PROG_H */
