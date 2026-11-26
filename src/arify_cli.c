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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "arif.h"
#include "arif_common.h"

// Forward declaration start
static void append_engines (char **, char const *);
static int  parse_options  (int, char *const []);
// Forward declaration end

static void
append_engines (
    char       **dest_ptr,
    char const  *src
) {
    char   *dest     = *dest_ptr;
    size_t  dest_len = dest == NULL ? 0 : strlen(dest);
    size_t  src_len  = strlen(src) + 1;

    *dest_ptr = dest = xrealloc(dest, src_len + dest_len + 1);
    if (dest_len > 0) {
        dest[dest_len++] = ',';
    }
    memcpy(dest + dest_len, src, src_len);
}

static int
parse_options (
    int         argc,
    char *const argv[]
) {
    char       *engines = NULL;
    char const *preload = ARIF_LIBDIR "/libarify" ARIF_SHLIB_SUFFIX;

    for (int opt; -1 != (opt = getopt(argc, argv, "e:f:p:l:n:V")); ) {
        switch (opt) {
          case 'e':
            append_engines(&engines, optarg);
            break;

          case 'f':
            setenv("ARIFY_FRONTEND", optarg, 1);
            break;

          case 'p':
            preload = optarg;
            break;

          case 'l':
            setenv("ARIFY_LOG_FILE", optarg, 1);
            break;

          case 'n':
            setenv("ARIFY_PAGE_SIZE", optarg, 1);
            break;

          case 'V':
            fprintf(stderr, "arify (ARIF %s)\n", ARIF_VER_STR);
            exit(EXIT_SUCCESS);

          default:
            exit(EXIT_FAILURE);
        }
    }

    if (engines != NULL) {
        setenv("ARIFY_ENGINES", engines, 1);
        free(engines);
    }

    char const *old_preload = xgetenv("LD_PRELOAD", NULL);
    if (old_preload == NULL) {
        setenv("LD_PRELOAD", preload, 1);
    } else {
        char *new_preload;
        xasprintf(&new_preload, "%s:%s", old_preload, preload);
        setenv("LD_PRELOAD", new_preload, 1);
        free(new_preload);
    }

    return optind;
}

int
main (
    int   argc,
    char *argv[]
) {
    char const *program = argv[0];
    xassert(program != NULL);

    argv += parse_options(argc, argv);
    if (argv[0] == NULL) {
        fprintf(stderr, "Usage: %s [options] pathname [args]\n\n"
                "See the arify(1) man page for details.\n", program);
        exit(EXIT_FAILURE);
    }
    if (-1 == execvp(argv[0], argv)) {
        arif_logf("execvp(): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
