/**
 * arif/src/arif.c - ARIF base library
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

#include "arif.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cand_page {
    struct cand_page *prev;
    struct cand_page *next;
    int               size;
    struct arif_cand  values[];
};

struct arif_ctx {
    struct arif_opts          opts;
    struct arif_engine const *engine;
    void                     *engine_data;

    // candidate pages
    struct cand_page *current_page;
    struct cand_page *last_page;
    int               page_num;
    bool              no_more_pages;

    // input text from previous query
    char const *old_line;
    int         old_offset;
    int         old_len;
};

// Forward declaration start
static struct cand_page *
              choose_candidate  (struct cand_page *, int);
static int    compare_text      (char const *, int, char const *, int);
static void   copy_candidate    (struct arif_cand *, struct arif_cand const *,
                                 int, arif_cand_disp_func *);
static char * disp_cand_default (char const *, int, char const *, int,
                                 int, int *);
static void   first_candidates  (struct arif_ctx *, char const *, int, int);
static void   free_page         (struct cand_page *);
static void   free_page_list    (struct cand_page *);
static int    last_num_offset   (char const *, int);
static struct cand_page *
              more_candidates   (struct arif_ctx *, int);
static struct cand_page *
              new_page          (struct arif_cand const *, int,
                                 arif_cand_disp_func *);
static void   new_pages         (struct arif_cand const *, int, int,
                                 arif_cand_disp_func *, struct cand_page **,
                                 struct cand_page **);
static void   set_old_line      (struct arif_ctx *, char const *, int, int);
// Forward declaration end

static struct cand_page *
choose_candidate (
    struct cand_page *page,
    int               idx
) {
    if (page == NULL) {
        return NULL;
    }
    if (idx < 1 || idx > page->size) {
        return NULL;
    }
    struct arif_cand *cand = &page->values[idx - 1];

    int size = sizeof(struct cand_page) + sizeof(struct arif_cand);
    struct cand_page *new_page = malloc(size);
    assert(new_page != NULL);
    *new_page = (struct cand_page) {
        .size = 1,
    };
    *new_page->values = *cand;

    cand->display = NULL;
    free_page_list(page);
    return new_page;
}

static inline int
compare_text (
    char const *text1,
    int         len1,
    char const *text2,
    int         len2
) {
    if (len1 != len2) {
        return -1;
    }
    return memcmp(text1, text2, len1);
}

static inline void
copy_candidate (
    struct arif_cand       *dest,
    struct arif_cand const *src,
    int                     idx,
    arif_cand_disp_func    *disp_cand
) {
    int display_len;
    char *display = disp_cand(src->text, src->text_len,
            src->display, src->display_len, idx + 1, &display_len);

    *dest = (struct arif_cand) {
        .text          = src->text,
        .text_len      = src->text_len,
        .replace_start = src->replace_start,
        .replace_len   = src->replace_len,
        .transform     = src->transform,
        .transform_len = src->transform_len,
        .display       = display,
        .display_len   = display_len,
    };
}

static char *
disp_cand_default (
    char const *text,
    int         len,
    char const *comment,
    int         comment_len,
    int         idx,
    int        *display_len_ptr
) {
    char const *fmt;
    int         display_len;
    if (comment == NULL) {
        fmt         = "[%d] %.*s";
        display_len = snprintf(NULL, 0, fmt, idx, len, text);
    } else {
        fmt         = "[%d] %.*s (%.*s)";
        display_len = snprintf(NULL, 0, fmt, idx, len, text,
                comment_len, comment);
    }
    assert(display_len >= 0);

    char *disp = malloc(sizeof(char) * (display_len + 1));
    assert(disp != NULL);

    if (comment == NULL) {
        sprintf(disp, fmt, idx, len, text);
    } else {
        sprintf(disp, fmt, idx, len, text, comment_len, comment);
    }

    *display_len_ptr = display_len;
    return disp;
}

static void
first_candidates (
    struct arif_ctx *ctx,
    char const      *line,
    int              offset,
    int              len
) {
    struct cand_page *current_page = NULL;
    struct cand_page *last_page    = NULL;

    int num_required = ctx->opts.page_size;
    struct arif_cand const *candidates;
    int num_fetched = ctx->engine->query(ctx->engine_data, line, offset, len,
            num_required, &candidates);

    ctx->no_more_pages = num_fetched < num_required;
    if (num_fetched > 0) {
        new_pages(candidates, num_fetched, num_required, ctx->opts.disp_cand,
                &current_page, &last_page);
    }

    free_page_list(ctx->current_page);
    ctx->current_page = current_page;
    ctx->last_page    = last_page;
    ctx->page_num     = 1;
}

static void
free_page(
    struct cand_page *page
) {
    for (int i = 0; i < page->size; ++i) {
        struct arif_cand *cand = page->values + i;
        free((char *) cand->display);
    }
    free(page);
}

static void
free_page_list (
    struct cand_page *head
) {
    if (head == NULL) {
        return;
    }
    struct cand_page *next, *prev = head->prev;
    // free forward
    for (struct cand_page *page = head; page != NULL; page = next) {
        next = page->next;
        free_page(page);
    }
    // free backward
    for (struct cand_page *page = prev; page != NULL; page = next) {
        next = page->prev;
        free_page(page);
    }
}

static inline int
last_num_offset (
    char const *text,
    int         len
) {
    while (--len >= 0) {
        if (!isdigit((unsigned char) text[len])) {
            break;
        }
    }
    return len + 1;
}

static struct cand_page *
more_candidates (
    struct arif_ctx *ctx,
    int              num_pages
) {
    if (ctx->no_more_pages) {
        return NULL;
    }
    struct cand_page *last_page = ctx->last_page;
    if (last_page == NULL) {
        return NULL;
    }

    int page_size      = ctx->opts.page_size;
    int last_page_size = last_page->size;
    int num_required   = num_pages * page_size;
    if (last_page_size < page_size) {
        num_required -= last_page_size;
    }

    struct arif_cand const *candidates;
    int num_fetched = ctx->engine->query(ctx->engine_data, NULL, 0, 0,
            num_required, &candidates);
    if (num_fetched < num_required) {
        ctx->no_more_pages = true;
        if (num_fetched == 0) {
            return NULL;
        }
    }

    arif_cand_disp_func *disp_cand = ctx->opts.disp_cand;

    while (num_fetched > 0 && last_page_size < page_size) {
        copy_candidate(last_page->values + last_page_size, candidates++,
                last_page_size, disp_cand);
        --num_fetched;
        ++last_page_size;
    }
    last_page->size = last_page_size;

    struct cand_page *first = last_page;
    new_pages(candidates, num_fetched, page_size, disp_cand, &first,
            &ctx->last_page);
    return first;
}

static struct cand_page *
new_page (
    struct arif_cand const *candidates,
    int                     num,
    arif_cand_disp_func    *disp_cand
) {
    int size = sizeof(struct cand_page) + sizeof(struct arif_cand) * num;
    struct cand_page *page = malloc(size);
    assert(page != NULL);

    *page = (struct cand_page) {
        .size = num,
    };
    for (int i = 0; i < num; ++i) {
        struct arif_cand const *src_cand  = candidates + i;
        struct arif_cand       *dest_cand = page->values + i;
        copy_candidate(dest_cand, src_cand, i, disp_cand);
    }
    return page;
}

static void
new_pages (
    struct arif_cand const  *candidates,
    int                      num_cands,
    int                      page_size,
    arif_cand_disp_func     *disp_cand,
    struct cand_page       **first_ptr,
    struct cand_page       **last_ptr
) {
    struct cand_page *first = NULL, *curr = NULL;
    struct cand_page *prev  = *first_ptr;
    for (; num_cands > 0; candidates += page_size, num_cands -= page_size) {
        curr = new_page(candidates,
                num_cands > page_size ? page_size : num_cands, disp_cand);
        if (first == NULL) {
            first = curr;
        }
        if (prev != NULL) {
            prev->next = curr;
            curr->prev = prev;
        }
        prev = curr;
    }
    *first_ptr = first;
    *last_ptr  = curr;
}

struct arif_ctx *
arif_ctx_create (
    struct arif_opts const *options
) {
    struct arif_ctx *ctx = malloc(sizeof(struct arif_ctx));
    assert(ctx != NULL);

    arif_cand_disp_func *disp_cand = options->disp_cand;
    if (disp_cand == NULL) {
        disp_cand = disp_cand_default;
    }

    *ctx = (struct arif_ctx) {
        .opts = {
            .disp_cand = disp_cand,
            .page_size = options->page_size,
        },
    };
    return ctx;
}

void
arif_ctx_destroy (
    struct arif_ctx *ctx
) {
    if (ctx == NULL) {
        return;
    }
    arif_set_engine(ctx, NULL, NULL);
    free(ctx);
}

int
arif_fetch (
    struct arif_ctx         *ctx,
    struct arif_cand const **candidates_ptr
) {
    struct cand_page *page = ctx->current_page;
    if (page == NULL) {
        return 0;
    }

    *candidates_ptr = page->values;
    return page->size;
}

int
arif_query (
    struct arif_ctx *ctx,
    char const      *line,
    int              offset,
    int              len
) {
    if (line == NULL || len < 1) {
        return 0;
    }
    char const *input     = line + offset;
    int         input_len = last_num_offset(input, len);
    char const *old_input = ctx->old_line + ctx->old_offset;

    if (0 != compare_text(old_input, ctx->old_len, input, input_len)) {
        // new text
        char *saved_line = malloc(sizeof(char) * (offset + input_len));
        assert(saved_line != NULL);

        memcpy(saved_line, line, offset + input_len);
        set_old_line(ctx, saved_line, offset, input_len);
        first_candidates(ctx, saved_line, offset, input_len);
    } else {
        // same text as old query
        if (input_len == len) {
            // not selecting candidate
            goto finish;
        }
        // select candidate
        int idx = atoi(input + input_len);
        struct cand_page *page = choose_candidate(ctx->current_page, idx);
        if (page == NULL) {
            // bad candidate index
            goto finish;
        }
        // tell engine that we selected a candidate
        int selected_idx = (ctx->page_num - 1) * ctx->opts.page_size + idx - 1;
        ctx->engine->query(ctx->engine_data, NULL, selected_idx, 0, 0, NULL);

        ctx->current_page  = page;
        ctx->last_page     = page;
        ctx->page_num      = 1;
        ctx->no_more_pages = false;

        set_old_line(ctx, NULL, 0, 0);
    }

  finish:
    return input_len;
}

int
arif_select_page (
    struct arif_ctx *ctx,
    int              idx
) {
    struct cand_page *page = ctx->current_page;
    if (idx > 0) {
        idx -= ctx->page_num;
    } else {
        idx = idx == 0 ? 1 : -1;
    }
    int page_num = ctx->page_num + idx;

    if (idx < 0) {
        do {
            page = page->prev;
            if (page == NULL) {
                return -1;
            }
        } while (++idx < 0);
    } else {
        while (idx-- > 0) {
            page = page->next;
            if (page == NULL) {
                page = more_candidates(ctx, idx + 1);
                if (page == NULL) {
                    return -1;
                }
            }
        }
    }

    ctx->current_page = page;
    ctx->page_num     = page_num;
    return page_num;
}

void
arif_set_engine (
    struct arif_ctx          *ctx,
    struct arif_engine const *engine,
    void                     *engine_data
) {
    free_page_list(ctx->current_page);
    ctx->current_page  = NULL;
    ctx->last_page     = NULL;
    ctx->page_num      = 0;
    ctx->no_more_pages = false;

    set_old_line(ctx, NULL, 0, 0);

    ctx->engine        = engine;
    ctx->engine_data   = engine_data;
}

static inline void
set_old_line (
    struct arif_ctx *ctx,
    char const      *old_line,
    int              old_offset,
    int              old_len
) {
    free((char *) ctx->old_line);
    ctx->old_line   = old_line;
    ctx->old_offset = old_offset;
    ctx->old_len    = old_len;
}
