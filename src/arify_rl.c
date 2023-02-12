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

#ifdef HAVE_READLINE

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>

#include "arif_defs.h"
#include "arif_rl.h"

struct arify_rl_engine {
    struct arif_engine const *engine;
    void                     *engine_data;
    struct arify_rl_engine   *next;
};

// Forward declaration start
static char ** complete           (char const *, int, int);
static void    disable_arify      (bool);
static void    display_hook       (char **, int, int);
static void    enable_arify       (void);

static void    get_engine_info    (struct arify_rl_engine const *,
                                   char const **, char const **);
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

struct arify_funmap_entry {
    char const        *name;
    rl_command_func_t *func;
};

struct arify_rl_ctx {
    struct arif_ctx        *ctx;
    struct arify_rl_engine *engines;
    struct arify_rl_engine *current_engine;
    bool                    enabled;
    char const             *old_basic_quote_chars;
    rl_completion_func_t   *old_comp_func;
    rl_compdisp_func_t     *old_disp_func;
    int                     old_ignore_duplicates;
    int                     old_query_items;
    int                     old_sort_matches;
};

struct arify_frontend const arify_frontend_readline = {
    .init     = initialize,
    .finalize = finalize,
};

static struct arify_funmap_entry const funmap_entries[] = {
    { "arify-toggle",      rlfunc_toggle       },
    { "arify-next-engine", rlfunc_next_engine  },
    { "arify-page-up",     rlfunc_page_up      },
    { "arify-page-down",   rlfunc_page_down    },
    { "arify-engine-info", rlfunc_engine_info  },
};

static struct arify_rl_ctx rlctx;

static char **
complete (
    char const *text,
    int         start,
    int         end
) {
    arify_debug_printf("%s", "complete");

    return arif_rl_complete(rlctx.ctx, text, start, end);
}

static void
disable_arify (
    bool supress_message
) {
    rl_basic_quote_characters          = rlctx.old_basic_quote_chars;
    rl_attempted_completion_function   = rlctx.old_comp_func;
    rl_completion_display_matches_hook = rlctx.old_disp_func;
    rl_completion_query_items          = rlctx.old_query_items;
    rl_ignore_completion_duplicates    = rlctx.old_ignore_duplicates;
    rl_sort_completion_matches         = rlctx.old_sort_matches;

    rlctx.enabled = false;
    if (!supress_message) {
        rlprintf("%s", "[arify] disabled");
    }
    arify_debug_printf("%s", "disabled");
}

static void
display_hook (
    char **matches,
    int    num,
    int    max_len
) {
    arify_debug_printf("%s", "display hook");

    arif_rl_display(rlctx.ctx, matches, num, max_len);
}

static void
enable_arify (void)
{
    if (rlctx.current_engine == NULL) {
        struct arify_rl_engine *entry = get_next_engine();
        if (entry == NULL) {
            arify_err_printf("%s", "no available engines");
            return;
        }
        rlctx.current_engine = rlctx.engines = entry;
        arif_set_engine(rlctx.ctx, entry->engine, entry->engine_data);
    }

    rlctx.old_basic_quote_chars = rl_basic_quote_characters;
    rlctx.old_comp_func         = rl_attempted_completion_function;
    rlctx.old_disp_func         = rl_completion_display_matches_hook;
    rlctx.old_query_items       = rl_completion_query_items;
    rlctx.old_ignore_duplicates = rl_ignore_completion_duplicates;
    rlctx.old_sort_matches      = rl_sort_completion_matches;

    // This is necessary, as rl_complete_internal() appends these characters
    // even with rl_completion_suppress_quote set to non-zero
    rl_basic_quote_characters          = "";
    rl_attempted_completion_function   = complete;
    rl_completion_display_matches_hook = display_hook;
    rl_completion_query_items          = 0;
    rl_ignore_completion_duplicates    = 0;
    rl_sort_completion_matches         = 0;

    char const *engine_name;
    get_engine_info(rlctx.current_engine, &engine_name, NULL);
    rlprintf("[arify] enabled (engine: %s)", engine_name);

    rlctx.enabled = true;
    arify_debug_printf("%s", "enabled");
}

static void
get_engine_info (
    struct arify_rl_engine const  *entry,
    char const                   **name_ptr,
    char const                   **description_ptr
) {
    char const *name, *description;
    if (name_ptr == NULL) {
        name_ptr = &name;
    }
    if (description_ptr == NULL) {
        description_ptr = &description;
    }

    struct arif_engine const *engine = entry->engine;
    if (engine->info != NULL) {
        engine->info(entry->engine_data, name_ptr, description_ptr);
    } else {
        *name_ptr        = "";
        *description_ptr = "";
    }
}

static void
finalize (
    void *ARIF_UNUSED_ARG(frontend_data)
) {
    if (rlctx.enabled) {
        disable_arify(true);
    }

    struct arify_rl_engine *next, *entry;
    for (entry = rlctx.engines; entry != NULL; entry = next) {
        next = entry->next;
        free(entry);
        if (next == rlctx.engines) {
            break;
        }
    }

    arify_debug_printf("%s", "readline frontend finalized");
}

static struct arify_rl_engine *
get_next_engine (void)
{
    void *engine_data;
    struct arif_engine const *engine = arify_next_engine(&engine_data);
    if (engine == NULL) {
        return NULL;
    }

    struct arify_rl_engine *entry = malloc(sizeof(struct arify_rl_engine));
    assert(entry != NULL);
    *entry = (struct arify_rl_engine) {
        .engine      = engine,
        .engine_data = engine_data,
    };
    return entry;
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

    rlctx.ctx = ctx;
    *frontend_data_ptr = &rlctx;

    arify_debug_printf("%s", "readline frontend initialized");
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

    char const *name, *description;
    get_engine_info(rlctx.current_engine, &name, &description);
    rlprintf("[arify] engine: %s\n\n%s", name, description);

    arify_debug_printf("%s", "print engine info");
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

    struct arify_rl_engine *entry = rlctx.current_engine;
    if (entry->next == NULL) {
        entry->next = get_next_engine();
        if (entry->next == NULL) {
            entry->next = rlctx.engines;
        }
    }
    rlctx.current_engine = entry = entry->next;
    arif_set_engine(rlctx.ctx, entry->engine, entry->engine_data);

    char const *engine_name;
    get_engine_info(entry, &engine_name, NULL);
    rlprintf("[arify] engine: %s", engine_name);

    arify_debug_printf("next engine: %s", engine_name);
    return 0;
}

static int
rlfunc_page_down (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (!rlctx.enabled) {
        return 0;
    }
    if (rl_inhibit_completion) {
        return 0;
    }
    int page = arif_select_page(rlctx.ctx, 0);
    if (page > 0) {
        rl_complete_internal('?');
    }
    arify_debug_printf("page down: %d", page);
    return 0;
}

static int
rlfunc_page_up (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (!rlctx.enabled) {
        return 0;
    }
    if (rl_inhibit_completion) {
        return 0;
    }
    int page = arif_select_page(rlctx.ctx, -1);
    if (page > 0) {
        rl_complete_internal('?');
    }
    arify_debug_printf("page up: %d", page);
    return 0;
}

static int
rlfunc_toggle (
    int ARIF_UNUSED_ARG(arg),
    int ARIF_UNUSED_ARG(key)
) {
    if (rlctx.enabled) {
        disable_arify(false);
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

#endif  // defined(HAVE_READLINE)
