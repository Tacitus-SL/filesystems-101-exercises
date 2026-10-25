#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include "solution.h"

static int is_pid_name(const char *s) {
    if (!*s) {
        return 0;
    }
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit(s[i])) {
            return 0;
        }
    }
    return 1;
}

static char **read_and_split(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        report_error(path, errno);
        char **empty = malloc(sizeof(char *));
        if (empty) empty[0] = NULL;
        return empty;
    }

    size_t cap = 512, len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        close(fd);
        char **empty = malloc(sizeof(char *));
        if (empty) empty[0] = NULL;
        return empty;
    }

    ssize_t n;
    while ((n = read(fd, buf + len, cap - len)) > 0) {
        len += n;
        if (len >= cap) {
            cap *= 2;
            char *new_buf = realloc(buf, cap);
            if (!new_buf) {
                free(buf);
                close(fd);
                char **empty = malloc(sizeof(char *));
                if (empty) empty[0] = NULL;
                return empty;
            }
            buf = new_buf;
        }
    }
    close(fd);

    if (n < 0) {
        report_error(path, errno);
        free(buf);
        char **empty = malloc(sizeof(char *));
        if (empty) empty[0] = NULL;
        return empty;
    }

    if (len == 0) {
        free(buf);
        char **arr = malloc(sizeof(char *));
        if (arr) arr[0] = NULL;
        return arr;
    }

    size_t count = 0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '\0') {
            count++;
        }
    }

    char **arr = malloc((count + 1) * sizeof(char *));
    if (!arr) {
        free(buf);
        char **empty = malloc(sizeof(char *));
        if (empty) empty[0] = NULL;
        return empty;
    }

    size_t idx = 0;
    for (size_t i = 0; i < len; ) {
        arr[idx++] = &buf[i];
        while (i < len && buf[i] != '\0') {
            i++;
        }
        i++;
    }
    arr[idx] = NULL;
    return arr;
}

void ps(void)
{
    DIR *dir = opendir("/proc");
    if (!dir) {
        report_error("/proc", errno);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_pid_name(entry->d_name)) {
            pid_t pid = (pid_t)atoi(entry->d_name);
            char path[512];

            snprintf(path, sizeof(path), "/proc/%s/exe", entry->d_name);
            char exe_buf[1024];
            ssize_t len = readlink(path, exe_buf, sizeof(exe_buf) - 1);
            if (len < 0) {
                report_error(path, errno);
            } else {
                exe_buf[len] = '\0';
            }
            const char *exe = NULL;
            if (len >= 0) {
                exe = exe_buf;
            }

            snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
            char **argv = read_and_split(path);

            snprintf(path, sizeof(path), "/proc/%s/environ", entry->d_name);
            char **envp = read_and_split(path);

            report_process(pid, exe, argv, envp);

            if (argv) {
                free(argv[0]);
                free(argv);
            }
            if (envp) {
                free(envp[0]);
                free(envp);
            }
        }
    }
    closedir(dir);
}