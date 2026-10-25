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

void lsof(void) {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        report_error("/proc", errno);
        return;
    }

    struct dirent *proc_entry;
    while ((proc_entry = readdir(proc_dir)) != NULL) {
        if (is_pid_name(proc_entry->d_name)) {
            char fd_path[512];
            snprintf(fd_path, sizeof(fd_path), "/proc/%s/fd", proc_entry->d_name);

            DIR *fd_dir = opendir(fd_path);
			if (!fd_dir) {
				report_error(fd_path, errno);
				continue;
			}
			
			struct dirent *fd_entry;
        
			while ((fd_entry = readdir(fd_dir)) != NULL) {
				if (strcmp(fd_entry->d_name, ".") == 0 || strcmp(fd_entry->d_name, "..") == 0) {
					continue;
				}

				char symlink_path[1024];
				snprintf(symlink_path, sizeof(symlink_path), "/proc/%s/fd/%s", proc_entry->d_name, fd_entry->d_name);

				char target_buf[1024];
				ssize_t len = readlink(symlink_path, target_buf, sizeof(target_buf) - 1);
				if (len < 0) {
					report_error(symlink_path, errno);
				} else {
					target_buf[len] = '\0';
					report_file(target_buf);
				}
			}
			closedir(fd_dir);
        }
    }
    closedir(proc_dir);
}