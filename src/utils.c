#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int is_today(time_t timestamp) {
    time_t now = time(NULL);
    if (now == (time_t) -1) {
        fprintf(stderr, "Could not get current time\n");
        return -1;
    }

    struct tm now_tm;
    if (localtime_r(&now, &now_tm) == NULL) {
        fprintf(stderr, "Could not convert to localtime: %s\n", strerror(errno));
        return -1;
    }

    struct tm day_start_tm = now_tm;
    day_start_tm.tm_hour   = 0;
    day_start_tm.tm_min    = 0;
    day_start_tm.tm_sec    = 0;
    time_t day_start       = mktime(&day_start_tm);

    struct tm day_end_tm = now_tm;
    day_end_tm.tm_hour   = 23;
    day_end_tm.tm_min    = 59;
    day_end_tm.tm_sec    = 59;
    time_t day_end       = mktime(&day_end_tm);

    return (timestamp >= day_start && timestamp <= day_end);
}
