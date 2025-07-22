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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <strings.h>

#include <rime_api.h>

#include "arif_common.h"
#include "arif_rime_quirks.h"

#define XK_Down  0xff54  // #include <X11/keysym.h>

struct engine_ctx {
    RimeSessionId      session;
    struct cand_entry *candidates;
};

// Forward declaration start
static void          arif_rime_finalize (void *);
static void          arif_rime_info     (void *, char const **, char const **);
static int           arif_rime_init     (void *, void **);
static int           arif_rime_query    (void *, char const *, int, int, int,
                                         struct arif_cand const **);
static int           copy_candidate     (struct arif_cand *, RimeContext *,
                                         char const *, int);
static int           init_rime          (void);
static void          finalize_rime      (void);
static void          free_candidates    (struct engine_ctx *);
static char const *  gen_description    (void);
static int           get_log_level      (char const *);
static char const ** get_modules        (char const *);
// Forward declaration end

struct cand_entry {
    struct cand_entry *next;
    int                num_candidates;
    struct arif_cand   candidates[];
};

struct arif_engine const arif_rime_engine = {
    .init     = arif_rime_init,
    .finalize = arif_rime_finalize,
    .info     = arif_rime_info,
    .query    = arif_rime_query,
};

static RimeApi     *rime_api;
static char        *rime_description;
static char const **rime_modules;

static void
arif_rime_finalize (
    void *engine_data
) {
    struct engine_ctx *ctx = engine_data;

    free_candidates(ctx);

    if (rime_api != NULL) {
        rime_api->destroy_session(ctx->session);
    }

    free(ctx);
}

static void
arif_rime_info (
    void        *ARIF_UNUSED_ARG(engine_data),
    char const **name_ptr,
    char const **description_ptr
) {
    if (name_ptr != NULL) {
        *name_ptr = "rime";
    }
    if (description_ptr != NULL) {
        *description_ptr = gen_description();
    }
}

static int
arif_rime_init (
    void  *ARIF_UNUSED_ARG(args),
    void **engine_data_ptr
) {
    int status = init_rime();
    if (status != 0) {
        return status;
    }

    struct engine_ctx *ctx = xmalloc(sizeof(struct engine_ctx));
    *ctx = (struct engine_ctx) {
        .session = 0,
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
    int                      num_candidates,
    struct arif_cand const **candidates_ptr
) {
    struct engine_ctx *ctx = engine_data;

    char *prefix     = NULL;
    int   prefix_len = 0;

    if (line != NULL) {
        free_candidates(ctx);
        rime_api->destroy_session(ctx->session);
        ctx->session = rime_api->create_session();

        char const *text = line + offset;
        for (int idx = 0; idx < len; ++idx) {
            if (!rime_api->process_key(ctx->session, text[idx], 0)) {
                return 0;
            }

            RIME_STRUCT(RimeStatus, rimestatus);
            if (!rime_api->get_status(ctx->session, &rimestatus)) {
                return 0;
            }
            bool is_composing = rimestatus.is_composing;
            rime_api->free_status(&rimestatus);
            if (is_composing) {
                continue;
            }

            // When `is_composing == false`, it means that
            // the previous input has triggered a commit.
            RIME_STRUCT(RimeCommit, rimecommit);
            if (!rime_api->get_commit(ctx->session, &rimecommit)) {
                continue;
            }
            int commit_len = strlen(rimecommit.text);
            prefix = xrealloc(prefix, prefix_len + commit_len);
            memcpy(prefix + prefix_len, rimecommit.text, commit_len);
            prefix_len += commit_len;
            rime_api->free_commit(&rimecommit);
        }
    } else if (candidates_ptr == NULL) {
        rime_api->select_candidate(ctx->session, offset);
        return 0;
    } else {
        if (!rime_api->find_session(ctx->session)) {
            return 0;
        }
    }

    size_t candidates_size = sizeof(struct arif_cand) * num_candidates;
    struct cand_entry *entries
            = xmalloc(sizeof(struct cand_entry) + candidates_size);
    entries->next = ctx->candidates;
    ctx->candidates = entries;

    int idx = 0;
    while (idx < num_candidates) {
        RIME_STRUCT(RimeContext, rimectx);
        if (!rime_api->get_context(ctx->session, &rimectx)) {
            break;
        }

        struct arif_cand *candidate = entries->candidates + idx;
        int result = copy_candidate(candidate, &rimectx, prefix, prefix_len);

        rime_api->free_context(&rimectx);

        if (result < 0) {
            // no candidates
            break;
        }
        ++idx;
        if (result > 0) {
            // last candidate
            break;
        }
        if (!rime_api->process_key(ctx->session, XK_Down, 0)) {
            break;
        }
    }

    if (idx == 0 && prefix_len > 0) {
        struct arif_cand *cand = &entries->candidates[0];

        *cand = (struct arif_cand) {
            .text        = prefix,
            .text_len    = prefix_len,
            .replace_len = len,
        };
        idx = 1;
    } else {
        free(prefix);
    }

    entries->num_candidates = idx;
    *candidates_ptr = entries->candidates;
    return idx;
}

static int
copy_candidate (
    struct arif_cand *dest,
    RimeContext      *src,
    char const       *prefix,
    int               prefix_len
) {
    RimeMenu *menu = &src->menu;
    int candidate_idx = menu->highlighted_candidate_index;
    if (candidate_idx >= menu->page_size) {
        return -1;
    }

    RimeCandidate   *candidate   = &menu->candidates[candidate_idx];
    RimeComposition *composition = &src->composition;

    char const *text = candidate->text;
    if (text == NULL) {
        text = "";
    }
    int text_len = strlen(text);

    char const *comment = candidate->comment;
    if (comment == NULL) {
        comment = "";
    }
    int comment_len = strlen(comment);

    char const *line     = composition->preedit;
    int         line_len = composition->length;

    char *buf = xmalloc(text_len + prefix_len + line_len + comment_len + 1);
    memcpy(buf,                                    text,    text_len);
    memcpy(buf + text_len,                         prefix,  prefix_len);
    memcpy(buf + text_len + prefix_len,            line,    line_len);
    memcpy(buf + text_len + prefix_len + line_len, comment, comment_len);

    // Distinguish "no comment" from "empty comment",
    // so that the frontend can handle them differently.
    if (candidate->comment == NULL) {
        comment = NULL;
    } else {
        comment = buf + text_len + line_len;
    }
    *dest = (struct arif_cand) {
        .text          = buf,
        .text_len      = text_len,
        .replace_start = prefix_len + composition->sel_start,
        .replace_len   = composition->sel_end - composition->sel_start,
        .transform     = buf + text_len,
        .transform_len = prefix_len + line_len,
        .display       = comment,
        .display_len   = comment_len,
    };

    return menu->is_last_page && candidate_idx + 1 == menu->page_size;
}

static void
free_candidates (
    struct engine_ctx *ctx
) {
    struct cand_entry *entry, *next;
    for (entry = ctx->candidates; entry != NULL; entry = next) {
        for (int idx = 0; idx < entry->num_candidates; ++idx) {
            struct arif_cand *candidate = entry->candidates + idx;
            free((char *) candidate->text);
        }

        next = entry->next;
        free(entry);
    }
    ctx->candidates = NULL;
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
    traits.min_log_level = get_log_level("ARIF_RIME_LOG_LEVEL");
    traits.modules       = get_modules("ARIF_RIME_MODULES");

    rime_api->setup(&traits);
    // Prevent glog (used by Rime for logging) from writing to stderr,
    // since it may break Readline output.
    arif_rime_glog_nostderr();
    rime_api->initialize(&traits);

    // Rime uses static C++ variables to store its states, which get destroyed
    // on exit.  Make sure we finalize Rime API before that.
    atexit(finalize_rime);

    rime_modules = traits.modules;
    return 0;
}

static void
finalize_rime (void)
{
    if (rime_api != NULL) {
        rime_api->finalize();
        rime_api = NULL;
    }
    if (rime_modules != NULL) {
        free((char *) rime_modules[0]);
        free(rime_modules);
        rime_modules = NULL;
    }
    free(rime_description);
}

static char const *
gen_description (void)
{
    if (rime_description != NULL) {
        return rime_description;
    }

    char const *fmt = "Example Rime integration for ARIF\n"
            "  Rime IME <https://rime.im/> version: %s\n";
    char const *rime_version = rime_api->get_version();
    if (rime_version == NULL) {
        rime_version = "N/A";
    }

    int desc_len = snprintf(NULL, 0, fmt, rime_version);
    assert(desc_len > 0);

    rime_description = xmalloc(sizeof(char) * (desc_len + 1));
    sprintf(rime_description, fmt, rime_version);

    return rime_description;
}

static int
get_log_level (
    char const *env_name
) {
    char const *log_level = xgetenv(env_name, "");

    if (0 == strcasecmp("INFO", log_level)) {
        return 0;
    }
    if (0 == strcasecmp("WARNING", log_level)) {
        return 1;
    }
    if (0 == strcasecmp("ERROR", log_level)) {
        return 2;
    }
    if (0 == strcasecmp("FATAL", log_level)) {
        return 3;
    }
    return 2;
}

static char const **
get_modules (
    char const *env_name
) {
    char const *modules_env = xgetenv(env_name, NULL);
    if (modules_env == NULL) {
        return NULL;
    }
    size_t modules_len = strlen(modules_env) + 1;

    char *modules_str = xmalloc(sizeof(char) * modules_len);
    memcpy(modules_str, modules_env, modules_len);

    char const **modules = NULL;
    size_t idx = 0;
    for (size_t modules_size = 4; ; *(modules_str++) = '\0') {
        if (modules == NULL || idx == modules_size - 1) {
            modules_size += modules_size / 2;
            modules = xrealloc(modules, sizeof(char const *) * modules_size);
        }
        modules[idx++] = modules_str;

        modules_str = strchr(modules_str, ',');
        if (modules_str == NULL) {
            break;
        }
    }
    modules[idx] = NULL;
    return modules;
}
