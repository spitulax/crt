#ifndef UTILS_H
#define UTILS_H

#include <time.h>

#define return_defer(v)                                                                            \
    do {                                                                                           \
        result = (v);                                                                              \
        goto defer;                                                                                \
    } while (0)

// returns -1 if failed
int is_today(time_t timestamp);

#endif /* ifndef UTILS_H */
