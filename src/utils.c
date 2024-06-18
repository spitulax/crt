#include "utils.h"
#include "memplus.h"
#include "prog.h"
#include <errno.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static const char *shell_names[] = {
    [SHELL_FISH] = "fish",
};

int is_today(time_t timestamp) {
    time_t now = time(NULL);
    if (now == (time_t) -1) {
        eprintfln("%s", "Could not get current time");
        return -1;
    }

    struct tm *day_start_tm = localtime(&now);
    if (day_start_tm == NULL) {
        eprintfln("Could not convert to localtime: %s", strerror(errno));
        return -1;
    }

    day_start_tm->tm_hour = 0;
    day_start_tm->tm_min  = 0;
    day_start_tm->tm_sec  = 0;
    time_t day_start      = mktime(day_start_tm);

    return timestamp >= day_start;
}

bool write_db(Prog *prog, Shell shell, time_t last_updated, int count) {
    bool          result  = true;
    sqlite3      *db      = NULL;
    sqlite3_stmt *stmt    = NULL;
    mp_String     db_path = mp_string_newf(prog->alloc, "%s/" DB_FILENAME, prog->data_home.cstr);

    if (sqlite3_open_v2(db_path.cstr, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) !=
        SQLITE_OK) {
        eprintfln("Failed to open sqlite3 database %s: %s", db_path.cstr, sqlite3_errmsg(db));
        return_defer(false);
    }

    mp_String statements[] = {
        mp_string_new(prog->alloc,
                      "CREATE TABLE IF NOT EXISTS shells (\n"
                      "   name TEXT PRIMARY KEY,\n"
                      "   last_updated INTEGER NOT NULL,\n"
                      "   count INTEGER NOT NULL\n"
                      ");"),
        mp_string_newf(prog->alloc,
                       "REPLACE INTO shells (name, last_updated, count)\n"
                       "VALUES ('%s', %ld, %d);",
                       shell_names[shell],
                       last_updated,
                       count),
    };

    for (size_t i = 0; i < sizeof(statements) / sizeof(*statements); ++i) {
        mp_String *statement = &statements[i];

        if (sqlite3_prepare_v2(db, statement->cstr, statement->size, &stmt, NULL) != SQLITE_OK) {
            eprintfln("Failed to compile SQL statements: %s", sqlite3_errmsg(db));
            return_defer(false);
        }

        int  ret;
        bool error = false;
        while ((ret = sqlite3_step(stmt)) != SQLITE_DONE) {
            switch (ret) {
                case SQLITE_ROW: {
                } break;
                case SQLITE_ERROR: {
                    eprintfln("SQLITE_ERROR: %s", sqlite3_errmsg(db));
                    error = true;
                } break;
                case SQLITE_MISUSE: {
                    eprintfln("%s", "SQLITE_MISUSE");
                    error = true;
                } break;
                case SQLITE_BUSY: {
                    eprintfln("%s", "SQLITE_BUSY");
                    error = true;
                } break;
                default: break;
            }
            if (error) break;
        }

        if (sqlite3_finalize(stmt) != SQLITE_OK) {
            eprintfln("Failed to finalize sqlite3 prepared statement for %s: %s",
                      db_path.cstr,
                      sqlite3_errmsg(db));
            return_defer(false);
        };
        if (error) return_defer(false);
    }


defer:
    if (sqlite3_close(db) != SQLITE_OK) {
        eprintfln("Failed to close sqlite3 database %s", db_path.cstr);
        return_defer(false);
    }
    return result;
}

bool read_db(Prog *prog, Shell shell, time_t *out_last_updated, int *out_count, bool check_only) {
    bool          result  = true;
    sqlite3      *db      = NULL;
    sqlite3_stmt *stmt    = NULL;
    mp_String     db_path = mp_string_newf(prog->alloc, "%s/" DB_FILENAME, prog->data_home.cstr);

    if (sqlite3_open_v2(db_path.cstr, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        if (!check_only)
            eprintfln("Failed to open sqlite3 database %s: %s", db_path.cstr, sqlite3_errmsg(db));
        return_defer(false);
    }

    mp_String statements =
        mp_string_newf(prog->alloc, "SELECT * FROM shells WHERE name='%s';", shell_names[shell]);

    if (sqlite3_prepare_v2(db, statements.cstr, statements.size, &stmt, NULL) != SQLITE_OK) {
        eprintfln("Failed to compile SQL statements: %s", sqlite3_errmsg(db));
        return_defer(false);
    }

    int ret;
    while ((ret = sqlite3_step(stmt)) != SQLITE_DONE) {
        switch (ret) {
            case SQLITE_ROW: {
                *out_last_updated = sqlite3_column_int64(stmt, 1);
                *out_count        = sqlite3_column_int(stmt, 2);
            } break;
            case SQLITE_ERROR: {
                eprintfln("SQLITE_ERROR: %s", sqlite3_errmsg(db));
                return_defer(false);
            } break;
            case SQLITE_MISUSE: {
                eprintfln("%s", "SQLITE_MISUSE");
                return_defer(false);
            } break;
            case SQLITE_BUSY: {
                eprintfln("%s", "SQLITE_BUSY");
                return_defer(false);
            } break;
            default: break;
        }
    }

defer:
    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        eprintfln("Failed to finalize sqlite3 prepared statement for %s: %s",
                  db_path.cstr,
                  sqlite3_errmsg(db));
        return_defer(false);
    };
    if (sqlite3_close(db) != SQLITE_OK) {
        eprintfln("Failed to close sqlite3 database %s", db_path.cstr);
        return_defer(false);
    }
    return result;
}
