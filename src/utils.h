#ifndef UTILS_H
#define UTILS_H

#include "prog.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define printfln(fmt, ...)  printf(fmt "\n", __VA_ARGS__)
#define eprintfln(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)

// returns -1 if failed
int is_today(time_t timestamp);

bool write_db(Prog *prog, Shell shell, time_t last_updated, int count);
bool read_db(Prog *prog, Shell shell, time_t *out_last_updated, int *out_count, bool check_only);

#endif /* ifndef UTILS_H */
