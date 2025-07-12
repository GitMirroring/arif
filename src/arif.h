/**
 * arif/src/arif.h - ARIF base library
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

#ifndef ARIF_H_
#define ARIF_H_

#define ARIF_VER_MAJOR  0
#define ARIF_VER_MINOR  1
#define ARIF_VER_PATCH  0

struct arif_cand;
struct arif_ctx;

typedef char *(arif_cand_disp_func) (
    char const *text,
    int         text_len,
    char const *comment,
    int         comment_len,
    int         idx,
    int        *display_len_ptr
);

typedef void (arif_engine_finalize_func) (
    void *engine_data
);

typedef void (arif_engine_info_func) (
    void        *engine_data,
    char const **name_ptr,
    char const **description_ptr
);

typedef int (arif_engine_init_func) (
    void  *args,
    void **engine_data_ptr
);

typedef int (arif_engine_query_func) (
    void                    *engine_data,
    char const              *line,
    int                      offset,
    int                      len,
    int                      num_candidates,
    struct arif_cand const **candidates_ptr
);

struct arif_cand {
    char const *text;
    int         text_len;
    int         replace_start;
    int         replace_len;
    char const *transform;
    int         transform_len;
    char const *display;
    int         display_len;
};

struct arif_engine {
    arif_engine_init_func     *init;
    arif_engine_finalize_func *finalize;
    arif_engine_info_func     *info;
    arif_engine_query_func    *query;
};

struct arif_opts {
    arif_cand_disp_func *disp_cand;
    int                  page_size;
};

struct arif_ctx *
arif_ctx_create (
    struct arif_opts const *options
);

void
arif_ctx_destroy (
    struct arif_ctx *ctx
);

int
arif_fetch (
    struct arif_ctx         *ctx,
    struct arif_cand const **candidates_ptr
);

int
arif_query (
    struct arif_ctx *ctx,
    char const      *line,
    int              offset,
    int              len
);

int
arif_select_page (
    struct arif_ctx *ctx,
    int              idx
);

void
arif_set_engine (
    struct arif_ctx          *ctx,
    struct arif_engine const *engine,
    void                     *engine_data
);

#endif  // !defined(ARIF_H_)
