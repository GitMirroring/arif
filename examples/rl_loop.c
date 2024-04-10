/**
 * arif/examples/rl_loop.c
 * ----
 *
 * Copyright (C) 2024  CismonX <admin@cismon.net>
 *
 * This file is part of ARIF, Another Readline Input Framework.
 *
 * ARIF is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ARIF is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARIF.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_READLINE

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

// Forward declaration start
static void print_usage (char const *);
static int  send_line   (char *, int, char *[]);
// Forward declaration end

static void
print_usage (
    char const *program
) {
    fprintf(stderr, "Usage: %s [options] pathname [args]\n\n", program);
    fputs("See the rl-loop(1) man page for details.\n", stderr);
}

static int
send_line (
    char *line,
    int   replace_idx,
    char *argv[]
) {
    int pfds[2];
    if (0 != pipe(pfds)) {
        fprintf(stderr, "pipe(): %s\n", strerror(errno));
        return -1;
    }

    pid_t child = fork();
    if (child < 0) {
        fprintf(stderr, "fork(): %s\n", strerror(errno));
        return -1;
    }

    if (child != 0) {
        close(pfds[0]);
        if (replace_idx == 0) {
            for (size_t line_len = rl_end; line_len > 0; ) {
                ssize_t nbytes = write(pfds[1], line, line_len);
                if (nbytes < 0) {
                    fprintf(stderr, "write(): %s\n", strerror(errno));
                    break;
                }
                line_len -= nbytes;
            }
        }
        close(pfds[1]);

        if (-1 == waitpid(child, NULL, 0)) {
            fprintf(stderr, "waitpid(): %s\n", strerror(errno));
            return -1;
        }
    } else {
        close(pfds[1]);
        if (-1 == dup2(pfds[0], STDIN_FILENO)) {
            fprintf(stderr, "dup2(): %s\n", strerror(errno));
            return -1;
        }
        close(pfds[0]);

        if (replace_idx != 0) {
            argv[replace_idx] = line;
        }
        if (0 != execvp(argv[0], argv)) {
            fprintf(stderr, "execvp(): %s: %s\n", argv[0], strerror(errno));
            return -1;
        }
    }
    return 0;
}

int
main (
    int   argc,
    char *argv[]
) {
    char const *name        = "rl-loop";
    char const *prompt      = "% ";
    int         replace_idx = 0;
    bool        send_empty  = false;
    for (int opt; -1 != (opt = getopt(argc, argv, "ei:n:p:")); ) {
        switch (opt) {
          case 'e':
            send_empty = true;
            break;
          case 'i':
            replace_idx = atoi(optarg);
            break;
          case 'n':
            name = optarg;
            break;
          case 'p':
            prompt = optarg;
            break;
          case '?':
          default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    argc -= optind;
    if (argc == 0) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (replace_idx < 0 || replace_idx >= argc) {
        fprintf(stderr, "%s: bad option -i, expected [0, %d), got %d\n",
                argv[0], argc, replace_idx);
        exit(EXIT_FAILURE);
    }
    argv += optind;
    rl_readline_name = name;

    for (char *line; ; free(line)) {
        line = readline(prompt);
        if (line == NULL) {
            break;
        }
        if (rl_end > 0) {
            add_history(line);
        } else {
            if (!send_empty) {
                continue;
            }
        }
        if (0 != send_line(line, replace_idx, argv)) {
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}

#endif  // defined(HAVE_READLINE)
