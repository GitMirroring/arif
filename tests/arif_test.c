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

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "arif.h"
#include "arif_dummy_engine.h"

int
main (
    int   argc,
    char *argv[]
) {
    int  buffer_size        = 64;
    int  page_size          = 5;
    bool gen_all_candidates = false;
    for (int opt; -1 != (opt = getopt(argc, argv, "ab:p:")); ) {
        switch (opt) {
          case 'a':
            gen_all_candidates = true;
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

    char *buffer = malloc(sizeof(char) * buffer_size);
    assert(buffer != NULL);

    struct arif_opts opts = {
        .page_size = page_size,
    };
    struct arif_ctx *ctx = arif_ctx_create(&opts);
    assert(ctx != NULL);

    struct arif_engine const *engine = &arif_dummy_engine;
    struct arif_dummy_engine_opts engine_opts = {
        .gen_all_candidates = gen_all_candidates,
    };
    void *engine_data;
    if (0 != engine->init(&engine_opts, &engine_data)) {
        exit(EXIT_FAILURE);
    }
    arif_set_engine(ctx, engine, engine_data);

    int exit_status = EXIT_SUCCESS;

    if (0 != setvbuf(stdin, NULL, _IONBF, 0)) {
        exit_status = EXIT_FAILURE;
        goto finish;
    }

    for (;;) {
        if (NULL == fgets(buffer, buffer_size, stdin)) {
            break;
        }
        char const *end = memchr(buffer, '\n', buffer_size);
        if (end == NULL) {
            end = memchr(buffer, '\0', buffer_size);
        }

        switch (buffer[0]) {
          case '\0':
          case '\n':
            continue;
          case ':': ;
            int page_num = arif_select_page(ctx, atoi(buffer + 1));
            printf("%d\n", page_num);
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
                fwrite(cand->display, 1, cand->display_len, stdout);
                puts("");
            }
            break;
          case '>': ;
            int len = arif_query(ctx, buffer, 1, end - buffer - 1);
            printf("%d\n", len);
            break;
          default:
            puts("?");
            break;
        }
    }

  finish:
    arif_ctx_destroy(ctx);
    engine->finalize(engine_data);
    free(buffer);
    exit(exit_status);
}
