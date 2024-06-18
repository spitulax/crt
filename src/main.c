#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "fish.h"
#include "prog.h"
#include "utils.h"

typedef enum {
    PARSE_ARGS_RESULT_FAILED,
    PARSE_ARGS_RESULT_TERMINATE,
    PARSE_ARGS_RESULT_SUCCESS,
} ParseArgsResult;

ParseArgsResult parse_args(Prog *prog, int argc, char **argv);
void            usage(void);

int main(int argc, char **argv) {
    int      result = EXIT_SUCCESS;
    mp_Arena arena;
    mp_arena_init(&arena);
    mp_Allocator alloc = mp_arena_allocator(&arena);

    static Prog prog = { 0 };
    prog.alloc       = &alloc;

    switch (parse_args(&prog, argc, argv)) {
        case PARSE_ARGS_RESULT_FAILED: {
            usage();
            return_defer(EXIT_FAILURE);
        }
        case PARSE_ARGS_RESULT_TERMINATE: {
            return_defer(EXIT_SUCCESS);
        }
        case PARSE_ARGS_RESULT_SUCCESS: break;
    }

    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        eprintfln("%s", "$HOME is not set");
        return -1;
    }

    const char *xdg_data_home_cstr = getenv("XDG_DATA_HOME");
    if (xdg_data_home_cstr == NULL) {
        prog.xdg_data_home = mp_string_newf(prog.alloc, "%s/.local/share", home_dir);
    } else {
        prog.xdg_data_home = mp_string_newf(prog.alloc, "%s", xdg_data_home_cstr);
    }
    prog.data_home = mp_string_newf(prog.alloc, "%s/" DB_DIR, prog.xdg_data_home.cstr);
    struct stat dir_stat;
    if (stat(prog.data_home.cstr, &dir_stat)) {
        const char *cmd = mp_string_newf(prog.alloc, "mkdir -p %s", prog.data_home.cstr).cstr;
        if (system(cmd) != 0) {
            eprintfln("Failed to create directory %s", prog.data_home.cstr);
            return_defer(EXIT_FAILURE);
        }
    }

#ifdef DEBUG
    if (prog.test) {
        printf("Nothing's here, yet\n");
        return_defer(EXIT_SUCCESS);
    }
#endif

    int count = 0;
    switch (prog.shell) {
        case SHELL_FISH: {
            count = count_fish(&prog);
        } break;
        case SHELL_COUNT: {
            assert(0 && "unreachable");
        } break;
    }
    if (count < 0) return_defer(EXIT_FAILURE);

    const char *message = prog.verbose ? "Commands run today: " : "";
    printf("%s%d\n", message, count);

defer:
    mp_arena_destroy(&arena);
    return result;
}

const char *next_args(int *argc, char ***argv) {
    if (*argc <= 0) {
        *argv = NULL;
        return NULL;
    }
    --(*argc);
    const char *curr = **argv;
    ++(*argv);
    return curr;
}

ParseArgsResult parse_args(Prog *prog, int argc, char **argv) {
    next_args(&argc, &argv);

    const char *meta_arg = next_args(&argc, &argv);
    if (meta_arg == NULL) return PARSE_ARGS_RESULT_FAILED;
    if (strcmp(meta_arg, "-h") == 0 || strcmp(meta_arg, "--help") == 0) {
        usage();
        return PARSE_ARGS_RESULT_TERMINATE;
    } else if (strcmp(meta_arg, "-v") == 0 || strcmp(meta_arg, "--version") == 0) {
        printf(PROG_NAME " " PROG_VERSION "\n");
        return PARSE_ARGS_RESULT_TERMINATE;
    } else if (strcmp(meta_arg, "fish") == 0) {
        prog->shell = SHELL_FISH;
#ifdef DEBUG
    } else if (strcmp(meta_arg, "test") == 0) {
        prog->test = true;
#endif
    } else {
        return PARSE_ARGS_RESULT_FAILED;
    }

    while (true) {
        const char *arg = next_args(&argc, &argv);
        if (arg == NULL) break;

        if (strcmp(arg, "--verbose") == 0) {
            prog->verbose = true;
        } else if (strcmp(arg, "--update") == 0) {
            prog->update = true;
        } else {
            return PARSE_ARGS_RESULT_FAILED;
        }
    }

    if (argc != 0) return PARSE_ARGS_RESULT_FAILED;

    return PARSE_ARGS_RESULT_SUCCESS;
}

void usage(void) {
    printf("Usage: %s shell [options]\n", PROG_NAME);
    printf("\n");
    printf("Shells:\n");
    printf("    fish\n");
    printf("Meta Options:\n");
    printf("    --help, -h          show help\n");
    printf("    --version, -v       show version\n");
    printf("Options:\n");
    // TODO:
    printf("    --update           show a message instead of just number\n");
    printf("    --verbose           show a message instead of just number\n");
}
