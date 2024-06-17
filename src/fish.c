#include "fish.h"
#include "memplus.h"
#include "utils.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int count_fish(Prog *prog) {
    int result = 0;

    const char *hist_name = getenv("fish_history");
    if (hist_name == NULL) {
        hist_name = "fish";
    }

    mp_String hist_path =
        mp_string_newf(prog->alloc, "%s/fish/%s_history", prog->xdg_data_home.cstr, hist_name);

    mp_String file_content;
    if (!mp_read_entire_file(prog->alloc, &file_content, hist_path.cstr)) {
        eprintfln("Failed to read %s: %s", hist_path.cstr, strerror(errno));
        return -1;
    }

    printf("%zu\n", file_content.size);

    return result;
}

/*int count_fish() {*/
/*    int    result      = 0;*/
/*    FILE  *hist_file   = NULL;*/
/*    char  *buffer      = NULL;*/
/*    char **lines_array = NULL;*/
/**/
/*    const char *xdg_data_home  = getenv("XDG_DATA_HOME");*/
/*    const char *fish_hist_path = "fish/fish_history";*/
/*    int         path_size      = snprintf(NULL, 0, "%s/%s", xdg_data_home, fish_hist_path);*/
/*    char        path[path_size + 1];*/
/*    snprintf(path, path_size + 1, "%s/%s", xdg_data_home, fish_hist_path);*/
/*    hist_file = fopen(path, "r");*/
/*    if (hist_file == NULL) {*/
/*        fprintf(stderr, "[WARNING] Could not open %s: %s\n", fish_hist_path, strerror(errno));*/
/*        return_defer(0);*/
/*    }*/
/**/
/*    fseek(hist_file, 0, SEEK_END);*/
/*    size_t hist_size = ftell(hist_file);*/
/*    fseek(hist_file, 0, SEEK_SET);*/
/*    buffer = malloc(hist_size + 1);*/
/*    if (buffer == NULL) {*/
/*        fprintf(stderr, "Could not allocate buffer for the file content\n");*/
/*        return_defer(-1);*/
/*    }*/
/**/
/*    size_t bytes_read = fread(buffer, 1, hist_size, hist_file);*/
/*    if (bytes_read != hist_size || ferror(hist_file) != 0) {*/
/*        fprintf(stderr, "Could not read %s\n", fish_hist_path);*/
/*        return_defer(-1);*/
/*    }*/
/*    buffer[hist_size] = '\0';*/
/**/
/*    // Initialize pointers that points to the start of each lines*/
/*    size_t line_count = 0;*/
/*    for (char *char_ptr = buffer; (char_ptr = strchr(char_ptr, '\n')) != NULL; ++char_ptr)*/
/*        ++line_count;*/
/*    lines_array          = malloc((line_count + 1) * sizeof(char *));*/
/*    size_t index         = 0;*/
/*    lines_array[index++] = buffer;*/
/*    for (char *char_ptr = buffer; (char_ptr = strchr(char_ptr, '\n')) != NULL;) {*/
/*        *char_ptr = '\0';*/
/*        ++char_ptr;*/
/*        if (*char_ptr != '\0') lines_array[index++] = char_ptr;*/
/*    }*/
/*    if (index != line_count) {*/
/*        fprintf(stderr, "Somehow the line counting was broken\n");*/
/*        return_defer(-1);*/
/*    }*/
/**/
/*    // We iterate from the last line up to quicken filtering*/
/*    // because fish stores new history to the bottom*/
/*    for (ssize_t i = line_count - 1; i >= 0; --i) {*/
/*        char *line = lines_array[i];*/
/*        if (strcmp(line, "  when: ") == 49) {*/
/*            time_t time  = atoi(line + 8);*/
/*            int    today = is_today(time);*/
/*            if (today)*/
/*                ++result;*/
/*            else if (!today)*/
/*                break;    // we break early because we know the following commands are not run
 * today*/
/*            else if (today == -1)*/
/*                return_defer(EXIT_FAILURE);*/
/*        }*/
/*    }*/
/**/
/*defer:*/
/*    free(lines_array);*/
/*    free(buffer);*/
/*    if (hist_file != NULL) fclose(hist_file);*/
/*    return result;*/
/*}*/
