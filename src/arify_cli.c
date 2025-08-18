/**
 * arif/src/arify_cli.c - CLI wrapper for the ARIF preload library
 * ----
 *
 * Copyright (C) 2023  CismonX <admin@cismon.net>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "arif.h"
#include "arif_common.h"

struct options {
    char *frontend;
    char *engines;
    char *log_file;
    char *page_size;
    char *preload;
};

// Forward declaration start
static void   append_str    (char const *, char **, char);
static char * concat_str    (char const *, char const *, char);
static int    parse_options (int, char *const [], struct options *);
static void   set_envs      (struct options *);
// Forward declaration end

static void
append_str (
    char const  *src,
    char       **dest_ptr,
    char         sep
) {
    char *dest = *dest_ptr;

    size_t src_len  = strlen(src) + 1;
    size_t dest_len = dest == NULL ? 0 : strlen(dest);

    *dest_ptr = dest = xrealloc(dest, src_len + dest_len + 1);

    if (dest_len > 0) {
        dest[dest_len++] = sep;
    }
    memcpy(dest + dest_len, src, src_len);
}

static char *
concat_str (
    char const *left,
    char const *right,
    char        sep
) {
    size_t left_len  = strlen(left);
    size_t right_len = strlen(right) + 1;

    char *dest = xmalloc(sizeof(char) * (left_len + right_len + 1));
    memcpy(dest, left, left_len);
    dest[left_len++] = sep;
    memcpy(dest + left_len, right, right_len);
    return dest;
}

static int
parse_options (
    int             argc,
    char *const     argv[],
    struct options *opts
) {
    for (int opt; -1 != (opt = getopt(argc, argv, "e:f:p:l:n:V")); ) {
        switch (opt) {
          case 'e':
            append_str(optarg, &opts->engines, ',');
            break;

          case 'f':
            opts->frontend = optarg;
            break;

          case 'p':
            opts->preload = optarg;
            break;

          case 'l':
            opts->log_file = optarg;
            break;

          case 'n':
            opts->page_size = optarg;
            break;

          case 'V':
            fprintf(stderr, "arify (ARIF %s)\n", ARIF_VER_STR);
            exit(EXIT_SUCCESS);

          default:
            exit(EXIT_FAILURE);
        }
    }
    return optind;
}

static void
set_envs (
    struct options *opts
) {
    if (opts->frontend != NULL) {
        setenv("ARIFY_FRONTEND", opts->frontend, 1);
    }
    if (opts->engines != NULL) {
        setenv("ARIFY_ENGINES", opts->engines, 1);
    }
    if (opts->log_file != NULL) {
        setenv("ARIFY_LOG_FILE", opts->log_file, 1);
    }
    if (opts->page_size != NULL) {
        setenv("ARIFY_PAGE_SIZE", opts->page_size, 1);
    }

    char const *old_preload = xgetenv("LD_PRELOAD", "");
    opts->preload = concat_str(old_preload, opts->preload, ':');
    setenv("LD_PRELOAD", opts->preload, 1);
}

int
main (
    int   argc,
    char *argv[]
) {
    char const *program = argv[0];
    assert(program != NULL);

    struct options opts = {
        .preload = ARIF_LIBDIR "/libarify" ARIF_SHLIB_SUFFIX,
    };
    argv += parse_options(argc, argv, &opts);
    set_envs(&opts);
    free(opts.engines);
    free(opts.preload);

    if (argv[0] == NULL) {
        fprintf(stderr, "Usage: %s [options] pathname [args]\n\n"
                "See the arify(1) man page for details.\n", program);
        exit(EXIT_FAILURE);
    }
    if (-1 == execvp(argv[0], argv)) {
        perror("execvp()");
        exit(EXIT_FAILURE);
    }
}
