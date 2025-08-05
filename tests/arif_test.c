/**
 * arif/tests/arif_test.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "arif.h"
#include "arif_common.h"
#include "arif_dummy_engine.h"

int
main (
    int   argc,
    char *argv[]
) {
    int buffer_size        = 64;
    int page_size          = 5;
    int gen_all_candidates = 0;
    for (int opt; -1 != (opt = getopt(argc, argv, "ab:p:")); ) {
        switch (opt) {
          case 'a':
            gen_all_candidates = 1;
            break;

          case 'b':
            buffer_size = atoi(optarg);
            break;

          case 'p':
            page_size = atoi(optarg);
            break;

          default:
            exit(EXIT_FAILURE);
        }
    }
    if (0 != setvbuf(stdin, NULL, _IONBF, 0)) {
        exit(EXIT_FAILURE);
    }

    struct arif_engine const *engine = &arif_dummy_engine;
    struct arif_dummy_engine_opts engine_opts = {
        .gen_all_candidates = gen_all_candidates,
    };
    void *engine_data;
    if (0 != engine->init(&engine_opts, &engine_data)) {
        exit(EXIT_FAILURE);
    }

    struct arif_opts const opts = {
        .page_size = page_size,
    };
    struct arif_ctx *ctx = arif_ctx_create(&opts);
    arif_set_engine(ctx, engine, engine_data);

    char *buffer = xmalloc(sizeof(char) * buffer_size);
    while (NULL != fgets(buffer, buffer_size, stdin)) {
        switch (buffer[0]) {
          case '\0':
          case '\n':
            continue;

          case ':':
            printf("%d\n", arif_select_page(ctx, atoi(buffer + 1)));
            break;

          case '<': ;
            struct arif_cand const *candidates;
            int num = arif_fetch(ctx, &candidates);
            if (num == 0) {
                puts("-");
                break;
            }
            for (int i = 0; i < num; ++i) {
                struct arif_cand const *cand = candidates + i;
                printf("%.*s\n", cand->display_len, cand->display);
            }
            break;

          case '>': ;
            char const *end = memchr(buffer, '\n', buffer_size);
            if (end == NULL) {
                end = memchr(buffer, '\0', buffer_size);
            }
            printf("%d\n", arif_query(ctx, buffer, 1, end - buffer - 1));
            break;

          default:
            puts("?");
            break;
        }
    }

    arif_ctx_destroy(ctx);
    engine->finalize(engine_data);
    free(buffer);
    exit(EXIT_SUCCESS);
}
