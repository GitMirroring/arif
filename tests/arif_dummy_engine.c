/**
 * arif/tests/arif_dummy_engine.c
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

#include "arif_dummy_engine.h"

#include <stdlib.h>

#include "arif.h"
#include "arif_common.h"

struct dummy_engine_ctx {
    char const *input;
    int         input_len;

    struct arif_cand *candidates;
    int               num_candidates;
    int               gen_all_candidates;
};

// Forward declaration start
static void dummy_engine_finalize (void *);
static void dummy_engine_info     (void *, char const **, char const **);
static int  dummy_engine_init     (void *, void **);
static int  dummy_engine_query    (void *, char const *, int, int, int,
                                   struct arif_cand const **);
static void free_candidates       (struct arif_cand *, int);
static void generate_candidate    (struct arif_cand *, char, int);
// Forward declaration end

struct arif_engine const arif_dummy_engine = {
    .init     = dummy_engine_init,
    .finalize = dummy_engine_finalize,
    .info     = dummy_engine_info,
    .query    = dummy_engine_query,
};

static void
dummy_engine_finalize (
    void *engine_data
) {
    struct dummy_engine_ctx *ctx = engine_data;
    if (ctx == NULL) {
        return;
    }
    free_candidates(ctx->candidates, ctx->num_candidates);
    free(ctx);
}

static void
dummy_engine_info (
    void        *ARIF_UNUSED_ARG(engine_data),
    char const **name_ptr,
    char const **description_ptr
) {
    if (name_ptr != NULL) {
        *name_ptr = "dummy";
    }
    if (description_ptr != NULL) {
        *description_ptr = "A dummy engine for testing.";
    }
}

static int
dummy_engine_init (
    void  *args,
    void **engine_data_ptr
) {
    struct arif_dummy_engine_opts const *opts = args;

    struct dummy_engine_ctx *ctx = xmalloc(sizeof(*ctx));
    *ctx = (struct dummy_engine_ctx) {
        .gen_all_candidates = opts != NULL ? opts->gen_all_candidates : 0,
    };
    *engine_data_ptr = ctx;
    return 0;
}

static int
dummy_engine_query (
    void                    *engine_data,
    char const              *line,
    int                      offset,
    int                      len,
    int                      num_candidates,
    struct arif_cand const **candidates_ptr
) {
    struct dummy_engine_ctx *ctx = engine_data;

    char const *input;
    int         old_candidates = 0;
    if (line == NULL) {
        input          = ctx->input;
        len            = ctx->input_len;
        old_candidates = ctx->num_candidates;
    } else if (candidates_ptr == NULL) {
        return 0;
    } else {
        ctx->input     = input = line + offset;
        ctx->input_len = len;

        free_candidates(ctx->candidates, ctx->num_candidates);
        ctx->candidates     = NULL;
        ctx->num_candidates = 0;
    }

    int candidates_left = len - ctx->num_candidates;
    if (ctx->gen_all_candidates || candidates_left < num_candidates) {
        num_candidates = candidates_left;
    }
    if (num_candidates == 0) {
        return 0;
    }
    int new_candidates = old_candidates + num_candidates;

    struct arif_cand *candidates = xrealloc(ctx->candidates,
            sizeof(struct arif_cand) * new_candidates);
    ctx->candidates     = candidates;
    ctx->num_candidates = new_candidates;

    for (int i = old_candidates; i < new_candidates; ++i) {
        generate_candidate(candidates + i, input[i], i);
    }

    *candidates_ptr = candidates + old_candidates;
    return num_candidates;
}

static void free_candidates (
    struct arif_cand *cand,
    int               num
) {
    if (cand == NULL) {
        return;
    }
    for (int i = 0; i < num; ++i) {
        free((char *) cand[i].text);
    }
    free(cand);
}

static void
generate_candidate (
    struct arif_cand *cand,
    char              ch,
    int               offset
) {
    int len = offset + 1;
    char *text = xmalloc(sizeof(char) * len);

    for (int i = 0; i < len; ++i) {
        text[i] = ch;
    }

    *cand = (struct arif_cand) {
        .text        = text,
        .text_len    = len,
        .replace_len = len,
    };
}
