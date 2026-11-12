/**
 * arif/src/arif_rime.c - Rime IME integration for ARIF
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

#include "arif_rime.h"

#include <string.h>

#include <strings.h>

#include <rime_api.h>

#include "arif_common.h"

struct engine_ctx {
    RimeSessionId     session;
    struct cand_page *cands;
};

// Forward declaration start
static int  cand_copy  (struct arif_cand *, RimeContext *, char const *, int);
static void cands_free (struct engine_ctx *);
static int  init_rime     (void);
static void finalize_rime (void);
static int  get_log_level (void);
// Forward declaration end

struct cand_page {
    struct cand_page *next;
    int               num_cands;
    struct arif_cand  cands[];
};

static RimeApi *rime_api;

// XXX: glog (used by Rime) may break Readline output by writing to stderr.
//      This is a hack to override and disable the corresponding flags
//      without adding glog and a C++ compiler as our dependency.
#ifndef ARIF_RIME_GLOG_ALSOLOGTOSTDERR
#  define ARIF_RIME_GLOG_ALSOLOGTOSTDERR  _ZN3fLB21FLAGS_alsologtostderrE
#endif
#ifndef ARIF_RIME_GLOG_STDERRTHRESHOLD
#  define ARIF_RIME_GLOG_STDERRTHRESHOLD  _ZN3fLI21FLAGS_stderrthresholdE
#endif

int ARIF_RIME_GLOG_ALSOLOGTOSTDERR;  // fLB::FLAGS_alsologtostderr
int ARIF_RIME_GLOG_STDERRTHRESHOLD;  // fLI::FLAGS_stderrthreshold

static int
cand_copy (
    struct arif_cand *dst,
    RimeContext      *rime_ctx,
    char const       *prefix,
    int               prefix_len
) {
    RimeMenu *menu = &rime_ctx->menu;
    int cand_idx = menu->highlighted_candidate_index;
    if (cand_idx >= menu->page_size) {
        return -1;
    }

    RimeCandidate   *rime_cand = &menu->candidates[cand_idx];
    RimeComposition *rime_comp = &rime_ctx->composition;

    char const *text = rime_cand->text;
    if (text == NULL) {
        text = "";
    }
    int text_len = strlen(text);

    if (prefix == NULL) {
        prefix = "";
    }

    char const *comment = rime_cand->comment;
    if (comment == NULL) {
        comment = "";
    }
    int comment_len = strlen(comment);

    char const *line     = rime_comp->preedit;
    int         line_len = rime_comp->length;

    char *buf = xmalloc(text_len + prefix_len + line_len + comment_len + 1);
    memcpy(buf,                                    text,    text_len);
    memcpy(buf + text_len,                         prefix,  prefix_len);
    memcpy(buf + text_len + prefix_len,            line,    line_len);
    memcpy(buf + text_len + prefix_len + line_len, comment, comment_len);

    // Distinguish "no comment" from "empty comment",
    // so that the frontend can handle them differently.
    if (rime_cand->comment == NULL) {
        comment = NULL;
    } else {
        comment = buf + text_len + line_len;
    }
    *dst = (struct arif_cand) {
        .text          = buf,
        .text_len      = text_len,
        .replace_start = prefix_len + rime_comp->sel_start,
        .replace_len   = rime_comp->sel_end - rime_comp->sel_start,
        .transform     = buf + text_len,
        .transform_len = prefix_len + line_len,
        .display       = comment,
        .display_len   = comment_len,
    };

    return menu->is_last_page && cand_idx + 1 == menu->page_size;
}

static void
cands_free (
    struct engine_ctx *ctx
) {
    struct cand_page *page, *next;
    for (page = ctx->cands; page != NULL; page = next) {
        for (int idx = 0; idx < page->num_cands; ++idx) {
            struct arif_cand *candidate = page->cands + idx;
            free((char *) candidate->text);
        }

        next = page->next;
        free(page);
    }
    ctx->cands = NULL;
}

static int
init_rime (void)
{
    if (rime_api != NULL) {
        return 0;
    }

    rime_api = rime_get_api();
    if (rime_api == NULL) {
        return -1;
    }

    RIME_STRUCT(RimeTraits, traits);
    traits.app_name               = "rime.arif";
    traits.distribution_name      = "Rime";
    traits.distribution_code_name = "arif-rime";
    traits.distribution_version   = ARIF_VER_STR;
    traits.shared_data_dir
            = xgetenv("ARIF_RIME_SHARED_DATA_DIR", "/usr/share/rime-data");
    traits.user_data_dir = xgetenv("ARIF_RIME_USER_DATA_DIR", NULL);
    traits.log_dir       = xgetenv("ARIF_RIME_LOG_DIR", "/tmp");
    traits.min_log_level = get_log_level();
    rime_api->setup(&traits);

    // Rime sets this to true during init
    ARIF_RIME_GLOG_ALSOLOGTOSTDERR = 0;
    // default value is ERROR (2) instead of FATAL
    ARIF_RIME_GLOG_STDERRTHRESHOLD = 3;

    rime_api->initialize(&traits);
    // Rime uses static C++ variables to store its states, which get destroyed
    // on exit.  Make sure we finalize Rime API before that.
    atexit(finalize_rime);
    return 0;
}

static void
finalize_rime (void)
{
    if (rime_api != NULL) {
        rime_api->finalize();
        rime_api = NULL;
    }
}

static int
get_log_level (void)
{
    char const *log_level = xgetenv("ARIF_RIME_LOG_LEVEL", "");

    if (0 == strcasecmp("INFO", log_level)) {
        return 0;
    }
    if (0 == strcasecmp("WARNING", log_level)) {
        return 1;
    }
    if (0 == strcasecmp("FATAL", log_level)) {
        return 3;
    }
    return 2;  // "ERROR"
}

static void
arif_rime_finalize (
    void *engine_data
) {
    struct engine_ctx *ctx = engine_data;

    cands_free(ctx);
    if (rime_api != NULL) {
        rime_api->destroy_session(ctx->session);
    }
    free(ctx);
}

static void
arif_rime_info (
    void        *ARIF_UNUSED_ARG(engine_data),
    char const **name_ptr,
    char const **desc_ptr
) {
    if (name_ptr != NULL) {
        *name_ptr = "rime";
    }
    if (desc_ptr != NULL) {
        *desc_ptr = "RIME <https://rime.im/> integration for ARIF";
    }
}

static int
arif_rime_init (
    void  *ARIF_UNUSED_ARG(args),
    void **engine_data_ptr
) {
    if (0 != init_rime()) {
        return -1;
    }

    struct engine_ctx *ctx = xmalloc(sizeof(struct engine_ctx));
    *ctx = (struct engine_ctx) {
        .session = rime_api->create_session(),
    };
    *engine_data_ptr = ctx;
    return 0;
}

static int
arif_rime_query (
    void                    *engine_data,
    char const              *line,
    int                      offset,
    int                      len,
    int                      num_cands,
    struct arif_cand const **cands_ptr
) {
    struct engine_ctx *ctx = engine_data;

    char *prefix     = NULL;
    int   prefix_len = 0;

    if (line != NULL) {
        cands_free(ctx);
        rime_api->clear_composition(ctx->session);

        RIME_STRUCT(RimeCommit, commit);
        if (rime_api->get_commit(ctx->session, &commit)) {
            rime_api->free_commit(&commit);
        }

        char const *text = line + offset;
        for (int idx = 0; idx < len; ++idx) {
            RIME_STRUCT(RimeStatus, rime_status);
            if (!rime_api->process_key(ctx->session, text[idx], 0) ||
                    !rime_api->get_status(ctx->session, &rime_status)
            ) {
                free(prefix);
                return 0;
            }

            int is_composing = rime_status.is_composing;
            rime_api->free_status(&rime_status);
            if (is_composing) {
                continue;
            }

            // When `is_composing == false`, it means that
            // the previous input has triggered a commit.
            if (!rime_api->get_commit(ctx->session, &commit)) {
                continue;
            }
            int commit_len = strlen(commit.text);
            prefix = xrealloc(prefix, prefix_len + commit_len);
            memcpy(prefix + prefix_len, commit.text, commit_len);
            prefix_len += commit_len;
            rime_api->free_commit(&commit);
        }
    } else if (cands_ptr == NULL) {
        rime_api->select_candidate(ctx->session, offset);
        return 0;
    }

    struct cand_page *page = xmalloc(sizeof(struct cand_page) +
            sizeof(struct arif_cand) * num_cands);
    page->next = ctx->cands;
    ctx->cands = page;

    int i = 0;
    while (i < num_cands) {
        RIME_STRUCT(RimeContext, rime_ctx);
        if (!rime_api->get_context(ctx->session, &rime_ctx)) {
            break;
        }

        int result = cand_copy(page->cands + i, &rime_ctx, prefix, prefix_len);
        rime_api->free_context(&rime_ctx);

        if (result < 0) {
            // no candidates
            break;
        }
        ++i;
        if (result > 0) {
            // last candidate
            break;
        }
        // 0xff54 == XK_Down (see X11/keysyms.h)
        if (!rime_api->process_key(ctx->session, 0xff54, 0)) {
            break;
        }
    }

    if (i == 0 && prefix_len > 0) {
        // Rime commits but no candidates.
        page->cands[0] = (struct arif_cand) {
            .text        = prefix,
            .text_len    = prefix_len,
            .replace_len = len,
        };
        i = 1;
    } else {
        free(prefix);
    }

    page->num_cands = i;
    *cands_ptr = page->cands;
    return i;
}

struct arif_engine const arif_rime_engine = {
    .init     = arif_rime_init,
    .finalize = arif_rime_finalize,
    .info     = arif_rime_info,
    .query    = arif_rime_query,
};
