#ifndef UTILS_H
#define UTILS_H

#include "memplus.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define printfln(fmt, ...)  printf(fmt "\n", __VA_ARGS__)
#define eprintfln(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)

// returns -1 if failed
int is_today(time_t timestamp);

// run a command and return stdout to `output`
bool run_cmd_stdout(mp_Allocator *alloc, mp_String *output, const char *cmd);

#endif /* ifndef UTILS_H */
