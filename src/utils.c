#include "utils.h"
#include "memplus.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int is_today(time_t timestamp) {
    time_t now = time(NULL);
    if (now == (time_t) -1) {
        eprintfln("%s", "Could not get current time");
        return -1;
    }

    struct tm *day_start_tm = localtime(&now);
    struct tm *day_end_tm   = localtime(&now);
    if (day_start_tm == NULL || day_end_tm == NULL) {
        eprintfln("Could not convert to localtime: %s\n", strerror(errno));
        return -1;
    }

    day_start_tm->tm_hour = 0;
    day_start_tm->tm_min  = 0;
    day_start_tm->tm_sec  = 0;
    time_t day_start      = mktime(day_start_tm);

    day_end_tm->tm_hour = 23;
    day_end_tm->tm_min  = 59;
    day_end_tm->tm_sec  = 59;
    time_t day_end      = mktime(day_end_tm);

    return (timestamp >= day_start && timestamp <= day_end);
}

bool run_cmd_stdout(mp_Allocator *alloc, mp_String *output, const char *cmd) {
    bool result = true;

    int pipe_fd[2];
    int read_pipe  = -1;
    int write_pipe = -1;
    int dev_null   = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0222);

    if (pipe(pipe_fd) == -1) {
        eprintfln("run_cmd: Failed to create pipes: %s", strerror(errno));
        return_defer(false);
    }
    read_pipe  = pipe_fd[0];
    write_pipe = pipe_fd[1];

    pid_t pid = fork();
    if (pid == -1) {
        eprintfln("run_cmd: Failed to fork child process: %s", strerror(errno));
        return_defer(false);
    } else if (pid == 0) {
        close(read_pipe);
        dup2(write_pipe, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        close(write_pipe);
        close(dev_null);
        execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
        eprintfln("run_cmd: Failed not run `%s`: %s", cmd, strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            eprintfln("run_cmd: `%s` could not terminate: %s", cmd, strerror(errno));
            return_defer(false);
        }

        // NOTE: we don't give an f about what's in stderr
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) return_defer(false);

        char    output_cstr[1024];
        ssize_t bytes_read      = read(read_pipe, output_cstr, sizeof(output_cstr) - 1);
        output_cstr[bytes_read] = '\0';
        *output                 = mp_string_new(alloc, output_cstr);
        return_defer(true);
    }

defer:
    if (read_pipe != -1 || write_pipe != -1) {
        close(read_pipe);
        close(write_pipe);
    };
    if (dev_null != -1) close(dev_null);
    return result;
}
