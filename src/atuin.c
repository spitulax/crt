#include "atuin.h"
#include "memplus.h"
#include "utils.h"
#include <errno.h>
#include <sqlite3.h>

int count_atuin(Prog *prog) {
    int           result = 0;
    sqlite3      *db     = NULL;
    sqlite3_stmt *stmt   = NULL;
    mp_String     db_path =
        mp_string_newf(prog->alloc, "%s/atuin/history.db", prog->xdg_data_home.cstr);

    if (sqlite3_open_v2(db_path.cstr, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        eprintfln("Failed to open sqlite3 database %s: %s", db_path.cstr, sqlite3_errmsg(db));
        return_defer(-1);
    }

    const char *statements = "SELECT timestamp FROM history ORDER BY timestamp DESC;";
    if (sqlite3_prepare_v2(db, statements, (int) strlen(statements), &stmt, NULL) != SQLITE_OK) {
        eprintfln("Failed to compile SQL statements: %s", sqlite3_errmsg(db));
        return_defer(-1);
    }

    int ret;
    int count      = 0;
    int past_count = 0;
    while ((ret = sqlite3_step(stmt)) != SQLITE_DONE) {
        bool past_today = false;
        switch (ret) {
            case SQLITE_ROW: {
                ++count;
                if (prog->update && !past_today) {
                    int64_t timestamp = sqlite3_column_int64(stmt, 0) / 1000000000;
                    int     today     = is_today(timestamp);
                    if (today) {
                        --past_count;
                    } else if (!today) {
                        past_today = true;
                    } else if (today < 0) {
                        return_defer(-1);
                    }
                }
            } break;
            case SQLITE_ERROR: {
                eprintfln("SQLITE_ERROR: %s", sqlite3_errmsg(db));
                return_defer(-1);
            } break;
            case SQLITE_MISUSE: {
                eprintfln("%s", "SQLITE_MISUSE");
                return_defer(-1);
            } break;
            case SQLITE_BUSY: {
                eprintfln("%s", "SQLITE_BUSY");
                return_defer(-1);
            } break;
            default: break;
        }
    }

    if (prog->update) past_count += count;

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
        if (!write_db(prog, SHELL_ATUIN, day_start, past_count)) return -1;
    } else {
        if (!read_db(prog, SHELL_ATUIN, &db_last_updated, &db_count, true)) {
            int today = is_today(db_last_updated);
            if (!today) {
                if (!write_db(prog, SHELL_ATUIN, now, count)) return -1;
            } else if (today < 0) {
                return -1;
            }
        }
    }
    if (!read_db(prog, SHELL_ATUIN, &db_last_updated, &db_count, false)) return -1;
    result = count - db_count;
    return_defer((result >= 0) ? result : 0);

defer:
    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        eprintfln("Failed to finalize sqlite3 prepared statement for %s: %s",
                  db_path.cstr,
                  sqlite3_errmsg(db));
        return_defer(-1);
    };
    if (sqlite3_close(db) != SQLITE_OK) {
        eprintfln("Failed to close sqlite3 database %s", db_path.cstr);
        return_defer(-1);
    }
    return result;
}
