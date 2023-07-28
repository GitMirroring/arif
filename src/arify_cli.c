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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include "arif.h"
#include "arif_defs.h"

#ifndef ARIFY_LIBDIR
#define ARIFY_LIBDIR  /usr/local/lib
#endif
#define ARIFY_PRELOAD_LIB_(libdir)  #libdir "/libarify" ARIF_SHLIB_SUFFIX
#define ARIFY_PRELOAD_LIB(libdir)   ARIFY_PRELOAD_LIB_(libdir)

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
static void   print_usage   (char const *) ARIF_NORETURN;
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

    *dest_ptr = dest = realloc(dest, src_len + dest_len + 1);
    assert(dest != NULL);

    dest[dest_len++] = sep;
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

    char *dest = malloc(sizeof(char) * (left_len + right_len + 1));
    assert(dest != NULL);

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
    while (1) {
        switch (getopt(argc, argv, "e:f:p:l:n:HV")) {
          case 'e':
            append_str(optarg, &opts->engines, ' ');
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
            fprintf(stderr, "arify (ARIF %d.%d.%d)\n",
                    ARIF_VER_MAJOR, ARIF_VER_MINOR, ARIF_VER_PATCH);
            exit(EXIT_SUCCESS);
          case -1:
            return optind;
          case '?':
          default:
            print_usage(argv[0]);
            break;
        }
    }
}

static void
print_usage (
    char const *program
) {
    fputs("Usage: ", stderr);
    fputs(program, stderr);
    fputs(" [options] filepath [args]\n\n", stderr);
    fputs("See the arify(1) man page for details.\n", stderr);
    exit(EXIT_FAILURE);
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

    char *old_preload = getenv("LD_PRELOAD");
    if (old_preload == NULL) {
        old_preload = "";
    }
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
        .preload = ARIFY_PRELOAD_LIB(ARIFY_LIBDIR),
    };
    argv += parse_options(argc, argv, &opts);
    set_envs(&opts);
    free(opts.engines);
    free(opts.preload);

    if (argv[0] == NULL) {
        fprintf(stderr, "%s: no executable file specified\n", program);
        print_usage(program);
    }
    if (-1 == execvp(argv[0], argv)) {
        fprintf(stderr, "%s: %s: %s\n", program, argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    }
}
