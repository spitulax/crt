#include "fish.h"
#include "memplus.h"
#include "prog.h"
#include "utils.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

mp_vector_create(Lines, char *);

int count_fish(Prog *prog) {
    const char *hist_name = getenv("fish_history");
    if (hist_name == NULL) hist_name = "fish";

    mp_String hist_path =
        mp_string_newf(prog->alloc, "%s/fish/%s_history", prog->xdg_data_home.cstr, hist_name);

    mp_String file_content;
    if (!mp_read_entire_file(prog->alloc, &file_content, hist_path.cstr)) {
        eprintfln("Failed to read %s: %s", hist_path.cstr, strerror(errno));
        return -1;
    }

    Lines lines;
    mp_vector_init(&lines, prog->alloc);
    char *char_ptr = file_content.cstr;
    for (;;) {
        if (*char_ptr != '\0') mp_append(&lines, char_ptr);
        char_ptr = strchr(char_ptr, '\n');
        if (char_ptr == NULL) break;
        *char_ptr = '\0';
        ++char_ptr;
    }

    int count      = 0;
    int past_count = 0;
    for (size_t i = 0; i < lines.size; ++i) {
        if (strcmp(mp_get(lines, i), "- cmd: ") > 0) ++count;
    }
    if (prog->update) {
        past_count = count;
        for (ssize_t i = lines.size - 1; i >= 0; --i) {
            char *line = mp_get(lines, i);
            if (strcmp(line, "  when: ") == 49) {
                time_t time  = atoi(line + 8);
                int    today = is_today(time);
                if (today) {
                    --past_count;
                } else if (!today) {
                    break;
                } else if (today < 0) {
                    return -1;
                }
            }
        }
    }

    time_t db_last_updated = 0;
    int    db_count        = 0;
    time_t now             = time(NULL);
    if (prog->update) {
        struct tm *day_start_tm = localtime(&now);
        if (day_start_tm == NULL) {
            eprintfln("Could not convert to localtime: %s", strerror(errno));
            return -1;
        }
        day_start_tm->tm_hour = 0;
        day_start_tm->tm_min  = 0;
        day_start_tm->tm_sec  = 0;
        time_t day_start      = mktime(day_start_tm);
        if (!write_db(prog, SHELL_FISH, day_start, past_count)) return -1;
    } else {
        if (!read_db(prog, SHELL_FISH, &db_last_updated, &db_count, true)) {
            int today = is_today(db_last_updated);
            if (!today) {
                if (!write_db(prog, SHELL_FISH, now, count)) return -1;
            } else if (today < 0) {
                return -1;
            }
        }
    }
    if (!read_db(prog, SHELL_FISH, &db_last_updated, &db_count, false)) return -1;

    int result = count - db_count;
    return (result >= 0) ? result : 0;
}
