#include "atuin.h"
#include "memplus.h"
#include "utils.h"
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

    return_defer(final_count(prog, count, past_count));

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
