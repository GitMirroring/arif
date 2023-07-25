/**
 * arif/examples/arif_rime.c - example Rime IME integration for ARIF
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

#ifdef HAVE_RIME

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <strings.h>

#include <rime_api.h>

#include "arif_defs.h"

#ifndef ARIF_RIME_APP_NAME
#  define ARIF_RIME_APP_NAME  "rime.arif"
#endif
#ifndef ARIF_RIME_DIST_NAME
#  define ARIF_RIME_DIST_NAME  "Rime"
#endif
#ifndef ARIF_RIME_DIST_CODE_NAME
#  define ARIF_RIME_DIST_CODE_NAME  "arif-rime"
#endif

#define VERSION_STR_(major, minor, patch)  #major "." #minor "." #patch
#define VERSION_STR(major, minor, patch)   VERSION_STR_(major, minor, patch)

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
static int           copy_candidate     (struct arif_cand *, RimeContext *);
static int           init_rime          (void);
static void          finalize_rime      (void);
static void          free_candidates    (struct engine_ctx *);
static char const *  gen_description    (void);
static char const *  get_env            (char const *, char const *);
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

    struct engine_ctx *ctx = malloc(sizeof(struct engine_ctx));
    assert(ctx != NULL);

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

    if (line != NULL) {
        free_candidates(ctx);
        rime_api->destroy_session(ctx->session);
        ctx->session = rime_api->create_session();

        char const *text = line + offset;
        for (int idx = 0; idx < len; ++idx) {
            if (!rime_api->process_key(ctx->session, text[idx], 0)) {
                return 0;
            }
        }
    } else if (candidates_ptr == NULL) {
        // TODO: handle candidate selection so that we can preserve a
        //       Rime session when possible.
        return 0;
    } else {
        if (!rime_api->find_session(ctx->session)) {
            return 0;
        }
    }

    size_t candidates_size = sizeof(struct arif_cand) * num_candidates;
    struct cand_entry *entries
            = malloc(sizeof(struct cand_entry) + candidates_size);
    assert(entries != NULL);
    entries->next = ctx->candidates;
    ctx->candidates = entries;

    int idx = 0;
    while (idx < num_candidates) {
        RIME_STRUCT(RimeContext, rimectx);
        if (!rime_api->get_context(ctx->session, &rimectx)) {
            break;
        }

        struct arif_cand *candidate = entries->candidates + idx;
        int result = copy_candidate(candidate, &rimectx);

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
    entries->num_candidates = idx;
    *candidates_ptr = entries->candidates;
    return idx;
}

static int
copy_candidate (
    struct arif_cand *dest,
    RimeContext      *src
) {
    RimeMenu        *menu        = &src->menu;
    RimeComposition *composition = &src->composition;
    int candidate_idx = menu->highlighted_candidate_index;
    if (candidate_idx >= menu->page_size) {
        return -1;
    }

    char const *text     = menu->candidates[candidate_idx].text;
    int         text_len = strlen(text);
    char const *line     = composition->preedit;
    int         line_len = strlen(line);

    char *buf = malloc(text_len + line_len);
    assert(buf != NULL);

    dest->text          = memcpy(buf, text, text_len);
    dest->len           = text_len;
    dest->replace_len   = composition->sel_end - composition->sel_start;
    dest->transform     = memcpy(buf + text_len, line, line_len);
    dest->transform_len = line_len;

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
    traits.app_name               = ARIF_RIME_APP_NAME;
    traits.distribution_name      = ARIF_RIME_DIST_NAME;
    traits.distribution_code_name = ARIF_RIME_DIST_CODE_NAME;
    traits.distribution_version
            = VERSION_STR(ARIF_VER_MAJOR, ARIF_VER_MINOR, ARIF_VER_PATCH);
    traits.shared_data_dir
            = get_env("ARIF_RIME_SHARED_DATA_DIR", "/usr/share/rime-data");
    traits.user_data_dir = get_env("ARIF_RIME_USER_DATA_DIR", NULL);
    traits.log_dir       = get_env("ARIF_RIME_LOG_DIR", "/tmp");
    traits.min_log_level = get_log_level("ARIF_RIME_LOG_LEVEL");
    traits.modules       = get_modules("ARIF_RIME_MODULES");

    rime_api->setup(&traits);
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

    rime_description = malloc(sizeof(char) * (desc_len + 1));
    assert(rime_description != NULL);
    sprintf(rime_description, fmt, rime_version);

    return rime_description;
}

static char const *
get_env (
    char const *name,
    char const *default_val
) {
    char const *val = getenv(name);
    if (val == NULL || val[0] == '\0') {
        val = default_val;
    }
    return val;
}

static int
get_log_level (
    char const *env_name
) {
    char const *log_level = get_env(env_name, "");

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
    char const *modules_env = get_env(env_name, NULL);
    if (modules_env == NULL) {
        return NULL;
    }
    size_t modules_len = strlen(modules_env) + 1;

    char *modules_str = malloc(sizeof(char) * modules_len);
    assert(modules_str != NULL);
    memcpy(modules_str, modules_env, modules_len);

    size_t modules_size = 4, idx = 0;
    char const **modules = NULL;

    char const *delim = " \n\t\r\v\f";
    for (char *module = strtok(modules_str, delim); module != NULL;
               module = strtok(NULL, delim)
    ) {
        if (modules == NULL || idx == modules_size - 1) {
            modules_size += modules_size / 2;
            modules = realloc(modules, sizeof(char const *) * modules_size);
            assert(modules != NULL);
        }
        modules[idx++] = module;
    }
    modules[idx] = NULL;
    return modules;
}

#endif  // defined(HAVE_RIME)
