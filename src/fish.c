#include "fish.h"
#include "memplus.h"
#include "prog.h"
#include "utils.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
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

    int count = 0;
    for (size_t i = 0; i < lines.size; ++i) {
        if (strcmp(mp_get(lines, i), "- cmd: ") > 0) ++count;
    }

    time_t db_last_updated = 0;
    int    db_count        = 0;
    if (!read_db(prog, SHELL_FISH, &db_last_updated, &db_count, true) ||
        !is_today(db_last_updated) || prog->update) {
        if (!write_db(prog, SHELL_FISH, time(NULL), count)) return -1;
    }
    if (!read_db(prog, SHELL_FISH, &db_last_updated, &db_count, false)) return -1;
    printfln("%d, %d", count, db_count);

    return count - db_count;
}
