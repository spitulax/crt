#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fish.h"
#include "utils.h"

#define PROG_NAME    "crt"
#define PROG_VERSION "v0.1.1"

typedef enum {
    SHELL_FISH
} Shell;

typedef struct {
    Shell shell;
    bool  verbose;
} Config;

typedef enum {
    PARSE_ARGS_RESULT_FAILED,
    PARSE_ARGS_RESULT_TERMINATE,
    PARSE_ARGS_RESULT_SUCCESS,
} ParseArgsResult;

ParseArgsResult parse_args(Config *config, int argc, char **argv);
void            usage();

int main(int argc, char **argv) {
    static Config config;

    switch (parse_args(&config, argc, argv)) {
        case PARSE_ARGS_RESULT_FAILED: {
            usage();
            return EXIT_FAILURE;
        }
        case PARSE_ARGS_RESULT_TERMINATE: {
            return EXIT_SUCCESS;
        }
        case PARSE_ARGS_RESULT_SUCCESS: break;
    }

    int count;
    switch (config.shell) {
        case SHELL_FISH: {
            count = count_fish();
        } break;
    }
    if (count < 0) return EXIT_FAILURE;

    const char *message = config.verbose ? "Commands run today: " : "";
    printf("%s%d\n", message, count);

    return EXIT_SUCCESS;
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

ParseArgsResult parse_args(Config *config, int argc, char **argv) {
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
        config->shell = SHELL_FISH;
    } else {
        return PARSE_ARGS_RESULT_FAILED;
    }

    while (true) {
        const char *arg = next_args(&argc, &argv);
        if (arg == NULL) break;

        if (strcmp(arg, "--verbose") == 0) {
            config->verbose = true;
        } else {
            return PARSE_ARGS_RESULT_FAILED;
        }
    }

    if (argc != 0) return PARSE_ARGS_RESULT_FAILED;

    return PARSE_ARGS_RESULT_SUCCESS;
}

void usage() {
    printf("Usage: %s shell [options]\n", PROG_NAME);
    printf("\n");
    printf("Shells:\n");
    printf("    fish\n");
    printf("Meta Options:\n");
    printf("    --help, -h          show help\n");
    printf("    --version, -v       show version\n");
    printf("Options:\n");
    printf("    --verbose           show a message instead of just number\n");
}
