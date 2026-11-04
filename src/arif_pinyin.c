/**
 * arif/src/arif_pinyin.c - libpinyin integration for ARIF
 * ----
 *
 * Copyright (C) 2026  CismonX <admin@cismon.net>
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

#include "arif_pinyin.h"

#include <stdbool.h>  // required for libpinyin
#include <string.h>

#include <pinyin.h>

#include "arif_common.h"

struct engine_ctx {
    pinyin_instance_t *inst;

    struct arif_cand  *cands_page;
    guint              cands_total;
    guint              cands_fetched;

    struct arif_cand punct_cand;
};

// Forward declaration start
static int  cand_replace_len (lookup_candidate_t const *, int);
static void cands_free       (struct engine_ctx *);
static void finalize_pinyin  (void);
static int  init_pinyin      (void);
static int  query_punct      (unsigned char, struct arif_cand *);
// Forward declaration end

typedef size_t (arif_pinyin_parse_func) (
    pinyin_instance_t *inst,
    char const        *buf
);

static pinyin_context_t       *pyctx;
static arif_pinyin_parse_func *parse_pinyin;

static int
cand_replace_len (
    lookup_candidate_t const *cand,
    int                       len
) {
    struct cand_internal {
        lookup_candidate_type_t candidate_type;
        gchar *phrase_string;
        phrase_token_t token;
        guint8 phrase_length;
        gint8 nbest_index;
        guint16 begin;
        guint16 end;
        guint32 freq;
    };

    // XXX: libpinyin does not expose the length of input text to be replaced
    //      by a candidate (can only be obtained when choosing candidate).
    //      This hack may no longer work if the upstream changes struct layout.
    struct cand_internal const *cand_i = (struct cand_internal const *) cand;

    if (cand_i->nbest_index >= 0) {
        // full-sentence candidate
        return len;
    }
    return cand_i->end - cand_i->begin;
}

static void
cands_free (
    struct engine_ctx *ctx
) {
    free(ctx->cands_page);
    ctx->cands_page    = NULL;
    ctx->cands_total   = 0;
    ctx->cands_fetched = 0;
}

static void
finalize_pinyin (void)
{
    if (pyctx == NULL) {
        return;
    }

    pinyin_save(pyctx);
    pinyin_fini(pyctx);
    pyctx = NULL;
}

static int
init_pinyin (void)
{
    if (pyctx != NULL) {
        return 0;
    }

    char const *sysdatadir = xgetenv("ARIF_PINYIN_SYSTEM_DATA_DIR",
            "/usr/lib/libpinyin/data");
    char const *userdatadir = xgetenv("ARIF_PINYIN_USER_DATA_DIR", NULL);
    pyctx = pinyin_init(sysdatadir, userdatadir);
    if (pyctx == NULL) {
        return -1;
    }

    pinyin_option_t flags = PINYIN_INCOMPLETE | PINYIN_CORRECT_ALL |
            USE_DIVIDED_TABLE | USE_RESPLIT_TABLE | DYNAMIC_ADJUST;
    pinyin_set_options(pyctx, flags);

    int scheme = atoi(xgetenv("ARIF_PINYIN_INPUT_SCHEME", ""));

    char const *mode = xgetenv("ARIF_PINYIN_INPUT_MODE", "full_pinyin");
    if (0 == strcmp("double_pinyin", mode)) {
        parse_pinyin = pinyin_parse_more_double_pinyins;
        if (scheme > 0) {
            pinyin_set_double_pinyin_scheme(pyctx, scheme);
        }
    } else if (0 == strcmp("chewing", mode)) {
        parse_pinyin = pinyin_parse_more_chewings;
        if (scheme > 0) {
            pinyin_set_zhuyin_scheme(pyctx, scheme);
        }
    } else /* if (0 == strcmp("full_pinyin", mode)) */ {
        parse_pinyin = pinyin_parse_more_full_pinyins;
        if (scheme > 0) {
            pinyin_set_full_pinyin_scheme(pyctx, scheme);
        }
    }

    char const *addons = xgetenv("ARIF_PINYIN_ADDON_LIBS", "");
    for (; ; ++addons) {
        int i = atoi(addons);
        if (i > 0) {
            pinyin_load_addon_phrase_library(pyctx, i);
        }

        addons = strchr(addons, ',');
        if (addons == NULL) {
            break;
        }
    }

    atexit(finalize_pinyin);
    return 0;
}

static int
query_punct (
    unsigned char     ch,
    struct arif_cand *buf
) {
    static char const *punct_table[128] = {
        ['~'] = "～", ['`'] = "·", ['!'] = "！", ['$'] = "￥",
        ['^'] = "……", ['('] = "（", [')'] = "）", ['_'] = "——",
        ['['] = "「", [']'] = "」", ['{'] = "『", ['}'] = "』",
        ['\\'] = "、", [';'] = "；", [':'] = "：",
        ['\''] = "‘’", ['"'] = "“”", ['<'] = "《", ['>'] = "》",
        [','] = "，", ['.'] = "。", ['?'] = "？",
    };

    if (0 != ch >> 7) {
        return -1;
    }
    char const *punct = punct_table[ch];
    if (punct == NULL) {
        return -1;
    }

    *buf = (struct arif_cand) {
        .text        = punct,
        .text_len    = strlen(punct),
        .replace_len = 1,
    };
    return 0;
}

static void
arif_pinyin_finalize (
    void *engine_data
) {
    struct engine_ctx *ctx = engine_data;

    cands_free(ctx);
    pinyin_free_instance(ctx->inst);
    free(ctx);
}

static void
arif_pinyin_info (
    void        *ARIF_UNUSED_ARG(engine_data),
    char const **name_ptr,
    char const **desc_ptr
) {
    if (name_ptr != NULL) {
        *name_ptr = "pinyin";
    }
    if (desc_ptr != NULL) {
        *desc_ptr = "libpinyin integration for ARIF\n";
    }
}

static int
arif_pinyin_init (
    void  *ARIF_UNUSED_ARG(args),
    void **engine_data_ptr
) {
    if (0 != init_pinyin()) {
        return -1;
    }

    pinyin_instance_t *inst = pinyin_alloc_instance(pyctx);
    if (inst == NULL) {
        return -1;
    }

    struct engine_ctx *ctx = xmalloc(sizeof(*ctx));
    *ctx = (struct engine_ctx) {
        .inst = inst,
    };
    *engine_data_ptr = ctx;
    return 0;
}

static int
arif_pinyin_query (
    void                    *engine_data,
    char const              *line,
    int                      offset,
    int                      len,
    int                      num_cands,
    struct arif_cand const **cands_ptr
) {
    struct engine_ctx *ctx = engine_data;

    pinyin_instance_t *inst = ctx->inst;
    if (line != NULL) {
        cands_free(ctx);
        pinyin_reset(inst);

        if (len == 1) {
            // libpinyin does not provide a built-in punctuation table.
            if (0 == query_punct(line[offset], &ctx->punct_cand)) {
                *cands_ptr = &ctx->punct_cand;
                return 1;
            }
        }

        char *buf = xmalloc(offset + len + 2);
        memcpy(buf, line, offset);
        buf[offset] = '\0';
        memcpy(buf + offset + 1, line + offset, len);
        buf[offset + 1 + len] = '\0';

        parse_pinyin(inst, buf + offset + 1);
        pinyin_guess_sentence_with_prefix(inst, buf);
        pinyin_guess_candidates(inst, 0,
                SORT_BY_PHRASE_LENGTH | SORT_BY_FREQUENCY);
        free(buf);

        if (!pinyin_get_n_candidate(inst, &ctx->cands_total)) {
            return 0;
        }
    } else if (cands_ptr == NULL) {
        lookup_candidate_t *cand;
        if (!pinyin_get_candidate(inst, offset, &cand)) {
            return 0;
        }
        pinyin_choose_candidate(inst, 0, cand);

        lookup_candidate_type_t cand_type;
        if (!pinyin_get_candidate_type(inst, cand, &cand_type)) {
            return 0;
        }

        if (cand_type == NBEST_MATCH_CANDIDATE) {
            guint8 nbest_idx;
            if (!pinyin_get_candidate_nbest_index(inst, cand, &nbest_idx)) {
                return 0;
            }
            pinyin_train(inst, nbest_idx);
        }
        return 0;
    }

    if ((guint) num_cands > ctx->cands_total - ctx->cands_fetched) {
        num_cands = ctx->cands_total - ctx->cands_fetched;
    }

    struct arif_cand *page = xmalloc(sizeof(struct arif_cand) * num_cands);
    ctx->cands_page = page;

    int i = 0;
    for (; i < num_cands; ++i) {
        lookup_candidate_t *cand;
        if (!pinyin_get_candidate(inst, ctx->cands_fetched + i, &cand)) {
            break;
        }

        char const *cand_str;
        if (!pinyin_get_candidate_string(inst, cand, &cand_str)) {
            break;
        }

        page[i] = (struct arif_cand) {
            .text        = cand_str,
            .text_len    = strlen(cand_str),
            .replace_len = cand_replace_len(cand, len),
        };
    }
    ctx->cands_fetched += i;
    *cands_ptr = page;
    return i;
}

struct arif_engine const arif_pinyin_engine = {
    .init     = arif_pinyin_init,
    .finalize = arif_pinyin_finalize,
    .info     = arif_pinyin_info,
    .query    = arif_pinyin_query,
};
