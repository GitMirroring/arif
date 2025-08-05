/**
 * arif/src/arify_rl.c - Readline frontend for the ARIF preload library
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

#include "arify_rl.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "arif_common.h"
#include "arif_rl.h"

struct arify_rl_engine {
    struct arif_engine const *impl;
    void                     *data;
    char const               *name;
    char const               *description;
    struct arify_rl_engine   *next;
};

// Forward declaration start
static char ** complete           (char const *, int, int);
static void    disable_arify      (int);
static void    display_hook       (char **, int, int);
static void    enable_arify       (void);
static void    finalize           (void *);
static struct arify_rl_engine *
               get_next_engine    (void);
static int     initialize         (struct arif_ctx *, void **);
static int     rlfunc_engine_info (int, int);
static int     rlfunc_next_engine (int, int);
static int     rlfunc_page_down   (int, int);
static int     rlfunc_page_up     (int, int);
static int     rlfunc_toggle      (int, int);
static void    rlprintf           (char const *, ...);
// Forward declaration end

ARIF_INTERNAL
struct arify_frontend const arify_frontend_readline = {
    .init     = initialize,
    .finalize = finalize,
};

static struct arify_funmap_entry {
    char const        *name;
    rl_command_func_t *func;
} const funmap_entries[] = {
    { "arify-toggle",      rlfunc_toggle      },
    { "arify-next-engine", rlfunc_next_engine },
    { "arify-page-up",     rlfunc_page_up     },
    { "arify-page-down",   rlfunc_page_down   },
    { "arify-engine-info", rlfunc_engine_info },
};

static struct arify_rl_ctx {
    struct arif_ctx        *ctx;
    struct arify_rl_engine *engines;
    struct arify_rl_engine *current_engine;
    char const             *word_break_chars;
    int                     enabled;
} rlctx;

static char **
complete (
    char const *text,
    int         start,
    int         end
) {
    arify_debugf("complete: %d %d %s", start, end, text);

    return arif_rl_complete(rlctx.ctx, text, start, end);
}

static void
disable_arify (
    int suppress_message
) {
    arif_rl_disable();
    rlctx.enabled = 0;
    if (!suppress_message) {
        rlprintf("%s", "[arify] disabled");
    }

    arify_debugf("%s", "disabled");
}

static void
display_hook (
    char **matches,
    int    num,
    int    max_len
) {
    arify_debugf("display_hook: %d %d", num, max_len);

    arif_rl_display(rlctx.ctx, matches, num, max_len);
}

static void
enable_arify (void)
{
    if (rlctx.current_engine == NULL) {
        struct arify_rl_engine *engine = get_next_engine();
        if (engine == NULL) {
            arify_errf("%s", "no available engines");
            return;
        }
        rlctx.current_engine = rlctx.engines = engine;
        arif_set_engine(rlctx.ctx, engine->impl, engine->data);

        if (NULL == xgetenv("ARIFY_RL_NO_AUTO_UNSETENV", NULL)) {
            unsetenv("ARIFY_LOG_FILE");
            setenv("ARIFY_FRONTEND", "none", 1);
        }
    }

    arif_rl_enable(complete, display_hook, rlctx.word_break_chars);
    rlctx.enabled = 1;
    rlprintf("[arify] enabled (engine: %s)", rlctx.current_engine->name);

    arify_debugf("%s", "enabled");
}

static void
finalize (
    void *ARIF_UNUSED_ARG(frontend_data)
) {
    if (rlctx.enabled) {
        disable_arify(1);
    }

    struct arify_rl_engine *next, *engine;
    for (engine = rlctx.engines; engine != NULL; engine = next) {
        next = engine->next;
        free(engine);
        if (next == rlctx.engines) {
            break;
        }
    }

    arify_debugf("%s", "arify_rl: finalized");
}

static struct arify_rl_engine *
get_next_engine (void)
{
    void *engine_data;
    struct arif_engine const *engine_impl = arify_next_engine(&engine_data);
    if (engine_impl == NULL) {
        return NULL;
    }
    char const *name        = "(no name)";
    char const *description = "(no description)";
    if (engine_impl->info != NULL) {
        engine_impl->info(engine_data, &name, &description);
    }

    struct arify_rl_engine *engine = xmalloc(sizeof(*engine));
    *engine = (struct arify_rl_engine) {
        .impl        = engine_impl,
        .data        = engine_data,
        .name        = name,
        .description = description,
    };
    return engine;
}

static int
initialize (
    struct arif_ctx  *ctx,
    void            **frontend_data_ptr
) {
    // We must not call rl_initialize() here
    int entries = sizeof(funmap_entries) / sizeof(funmap_entries[0]);
    for (int idx = 0; idx < entries; ++idx) {
        struct arify_funmap_entry const *entry = funmap_entries + idx;
        rl_add_funmap_entry(entry->name, entry->func);
    }

    rlctx.ctx              = ctx;
    rlctx.word_break_chars = xgetenv("ARIFY_RL_WORD_BREAK_CHARS", " \t\n");

    *frontend_data_ptr = &rlctx;
    arify_debugf("%s", "arify_rl: initialized");
    return 0;
}

static int
rlfunc_engine_info (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (!rlctx.enabled) {
        return 0;
    }

    struct arify_rl_engine const *engine = rlctx.current_engine;
    rlprintf("[arify] engine: %s\n\n%s", engine->name, engine->description);

    arify_debugf("print engine info: %s", engine->name);
    return 0;
}

static int
rlfunc_next_engine (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (!rlctx.enabled) {
        return 0;
    }

    struct arify_rl_engine *engine = rlctx.current_engine;
    if (engine->next == NULL) {
        engine->next = get_next_engine();
        if (engine->next == NULL) {
            engine->next = rlctx.engines;
        }
    }
    rlctx.current_engine = engine = engine->next;
    arif_set_engine(rlctx.ctx, engine->impl, engine->data);

    char const *name = rlctx.current_engine->name;
    rlprintf("[arify] engine: %s", name);

    arify_debugf("next engine: %s", name);
    return 0;
}

static int
rlfunc_page_down (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (!rlctx.enabled || rl_inhibit_completion) {
        return 0;
    }
    int page = arif_select_page(rlctx.ctx, 0);
    if (page > 0) {
        rl_complete_internal('?');
    }

    arify_debugf("page down: %d", page);
    return 0;
}

static int
rlfunc_page_up (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (!rlctx.enabled || rl_inhibit_completion) {
        return 0;
    }
    int page = arif_select_page(rlctx.ctx, -1);
    if (page > 0) {
        rl_complete_internal('?');
    }

    arify_debugf("page up: %d", page);
    return 0;
}

static int
rlfunc_toggle (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (rlctx.enabled) {
        disable_arify(0);
    } else {
        enable_arify();
    }
    return 0;
}

static void
rlprintf (
    char const *fmt,
    ...
) {
    rl_crlf();

    va_list args;
    va_start(args, fmt);
    vfprintf(rl_outstream, fmt, args);
    va_end(args);

    rl_crlf();
    rl_forced_update_display();
}
