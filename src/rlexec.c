/**
 * arif/src/rlexec.c
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

#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "arif_common.h"

// Forward declaration start
static void on_signal (int);
static int  send_line (char *, int, char *[]);
// Forward declaration end

static volatile sig_atomic_t caught_signum;
static volatile sig_atomic_t in_readline;

static sigjmp_buf jmpbuf;

static void
on_signal (
    int s
) {
    caught_signum = s;

    if (in_readline) {
        in_readline = 0;
        siglongjmp(jmpbuf, 1);
    }
}

static int
send_line (
    char *line,
    int   replace_idx,
    char *argv[]
) {
    int pfds[2];
    if (0 != pipe(pfds)) {
        perror("pipe()");
        return -1;
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork()");
        goto err1;
    }

    if (child != 0) {
        close(pfds[0]);
        if (replace_idx == 0) {
            for (size_t line_len = strlen(line); line_len > 0; ) {
                ssize_t nbytes = write(pfds[1], line, line_len);
                if (nbytes < 0) {
                    perror("write()");
                    break;
                }
                line     += nbytes;
                line_len -= nbytes;
            }
        }
        close(pfds[1]);

        while (-1 == waitpid(child, NULL, 0)) {
            if (errno != EINTR) {
                perror("waitpid()");
                return -1;
            }
            kill(child, caught_signum);
        }
    } else {
        close(pfds[1]);
        if (-1 == dup2(pfds[0], STDIN_FILENO)) {
            perror("dup2()");
            goto err0;
        }
        close(pfds[0]);

        if (replace_idx != 0) {
            argv[replace_idx] = line;
        }
        if (0 != execvp(argv[0], argv)) {
            perror("execvp()");
            return -1;
        }
    }
    return 0;

  err1:
    close(pfds[1]);

  err0:
    close(pfds[0]);
    return -1;
}

int
main (
    int   argc,
    char *argv[]
) {
    char const *name        = "rlexec";
    char const *prompt      = "% ";
    int         replace_idx = 0;
    int         send_empty  = 0;
    for (int opt; -1 != (opt = getopt(argc, argv, "ei:n:p:")); ) {
        switch (opt) {
          case 'e':
            send_empty = 1;
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

          default:
            exit(EXIT_FAILURE);
        }
    }
    argc -= optind;
    if (argc == 0) {
        fprintf(stderr, "Usage: %s [options] pathname [args]\n\n"
                "See the rlexec(1) man page for details.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (replace_idx < 0 || replace_idx >= argc) {
        fprintf(stderr, "%s: bad option -i, expected [0, %d), got %d\n",
                argv[0], argc, replace_idx);
        exit(EXIT_FAILURE);
    }
    argv += optind;
    rl_readline_name = name;

    using_history();
    stifle_history(1024);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);

    struct sigaction sact = {
        .sa_handler = on_signal,
        .sa_mask    = mask,
    };
    sigaction(SIGINT, &sact, NULL);
    sigaction(SIGTERM, &sact, NULL);
    sigaction(SIGHUP, &sact, NULL);

    rl_catch_signals = 0;
    char *line = NULL;
    if (0 != sigsetjmp(jmpbuf, 1)) {
        if (line != NULL) {
            goto handle_line;
        }

        rl_free_line_state();
        rl_cleanup_after_signal();
        rl_crlf();
        rl_clear_visible_line();

        if (caught_signum != SIGINT || rl_end == 0) {
            exit(EXIT_FAILURE);
        }
        caught_signum = 0;
    }

    for (; caught_signum == 0; free(line), line = NULL) {
        in_readline = 1;
        line = readline(prompt);
        in_readline = 0;
        if (line == NULL) {
            break;
        }

      handle_line:
        if (line[0] != '\0') {
            add_history(line);
        } else {
            if (!send_empty) {
                continue;
            }
        }
        send_line(line, replace_idx, argv);
    }
    exit(caught_signum == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
