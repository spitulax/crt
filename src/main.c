#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROG_NAME    "crt"
#define PROG_VERSION "v0.1.1"

#define return_defer(v)                                                                            \
    do {                                                                                           \
        result = (v);                                                                              \
        goto defer;                                                                                \
    } while (0)

typedef enum {
    SHELL_FISH
} Shell;

typedef struct {
    int    result;
    FILE  *hist_file;
    char  *buffer;
    char **lines_ptr;
    int    count;
} State;

void state_free(State *self);

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
int             is_today(time_t timestamp);

int count_fish(State *state);

int main(int argc, char **argv) {
    int           result = EXIT_SUCCESS;
    static Config config;
    static State  state = {
         .result    = EXIT_SUCCESS,
         .hist_file = NULL,
         .buffer    = NULL,
         .lines_ptr = NULL,
         .count     = 0,
    };

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

    switch (config.shell) {
        case SHELL_FISH: {
            if (count_fish(&state) == EXIT_FAILURE) return_defer(EXIT_FAILURE);
        } break;
    }

    const char *message = config.verbose ? "Commands run today: " : "";
    printf("%s%d\n", message, state.count);

defer:
    state_free(&state);
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

void state_free(State *self) {
    if (self->hist_file != NULL) fclose(self->hist_file);
    if (self->buffer != NULL) free(self->buffer);
    if (self->lines_ptr != NULL) free(self->lines_ptr);
}

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

int count_fish(State *state) {
    int result = EXIT_SUCCESS;

    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home == NULL) {
        fprintf(stderr, "XDG_DATA_HOME is not set\n");
        // none of state's fields should be initialized yet but just to make sure
        state_free(state);
        return EXIT_FAILURE;
    }
    const char *fish_hist_path = "fish/fish_history";
    int         path_size      = snprintf(NULL, 0, "%s/%s", xdg_data_home, fish_hist_path);
    char        path[path_size + 1];
    snprintf(path, path_size + 1, "%s/%s", xdg_data_home, fish_hist_path);
    state->hist_file = fopen(path, "r");
    if (state->hist_file == NULL) {
        fprintf(stderr, "Could not open %s: %s\n", fish_hist_path, strerror(errno));
        return_defer(EXIT_FAILURE);
    }

    fseek(state->hist_file, 0, SEEK_END);
    size_t hist_size = ftell(state->hist_file);
    fseek(state->hist_file, 0, SEEK_SET);
    state->buffer = malloc(hist_size + 1);
    if (state->buffer == NULL) {
        fprintf(stderr, "Could not allocate buffer for the file content\n");
        return_defer(EXIT_FAILURE);
    }

    size_t bytes_read = fread(state->buffer, 1, hist_size, state->hist_file);
    if (bytes_read != hist_size || ferror(state->hist_file) != 0) {
        fprintf(stderr, "Could not read %s\n", fish_hist_path);
        return_defer(EXIT_FAILURE);
    }
    state->buffer[hist_size] = '\0';

    // Initialize pointers that points to the start of each lines
    size_t line_count = 0;
    for (char *char_ptr = state->buffer; (char_ptr = strchr(char_ptr, '\n')) != NULL; ++char_ptr)
        ++line_count;
    state->lines_ptr = malloc((line_count + 1) * sizeof(char *));
    if (state->lines_ptr == NULL) {
        fprintf(stderr, "Could not allocate lines pointer\n");
        return_defer(EXIT_FAILURE);
    }
    size_t index              = 0;
    state->lines_ptr[index++] = state->buffer;
    for (char *char_ptr = state->buffer; (char_ptr = strchr(char_ptr, '\n')) != NULL;) {
        *char_ptr = '\0';
        ++char_ptr;
        if (*char_ptr != '\0') state->lines_ptr[index++] = char_ptr;
    }
    if (index != line_count) {
        fprintf(stderr, "Somehow the line counting was broken\n");
        return_defer(EXIT_FAILURE);
    }

    // We iterate from the last line up to quicken filtering
    // because fish stores new history to the bottom
    for (ssize_t i = line_count - 1; i >= 0; --i) {
        char *line = state->lines_ptr[i];
        if (strcmp(line, "  when: ") == 49) {
            time_t time  = atoi(line + 8);
            int    today = is_today(time);
            if (today)
                ++state->count;
            else if (!today)
                break;    // we break early because we know the following commands are not run today
            else if (today == -1)
                return_defer(EXIT_FAILURE);
        }
    }

defer:
    return result;
}
