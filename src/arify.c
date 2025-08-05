/**
 * arif/src/arify.c - ARIF preload library
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

#include "arify.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include "arif.h"
#include "arif_common.h"

#ifdef ENABLE_ARIF_READLINE
#  include "arify_rl.h"
#endif

#ifndef ARIF_CTOR
#  error "__attribute__((constructor)) not supported"
#endif

#ifndef ARIFY_MAX_PAGE_SIZE
#  define ARIFY_MAX_PAGE_SIZE  20
#endif

struct arify_engine {
    char const               *lib_name;
    char const               *var_name;
    void                     *handle;
    struct arif_engine const *impl;
    void                     *data;
    struct arify_engine      *next;
};

// Forward declaration start
static int  config_engines   (void);
static int  config_frontend  (void);
static int  config_log_file  (void);
static int  config_page_size (void);
static void finalize         (void);
static void finalize_engines (struct arify_engine *);
static int  init_config      (void);
static void initialize       (void) ARIF_CTOR;
static int  load_engine      (struct arify_engine *);
// Forward declaration end

static struct arify_ctx {
    struct arify_engine         *engines;
    struct arify_engine         *current_engine;
    struct arify_frontend const *frontend;
    void                        *frontend_data;
    struct arif_ctx             *ctx;
    int                          log_fd;
    int                          page_size;
    pid_t                        pid;
} ctx = { .log_fd = -1 };

static int
config_engines (void)
{
    char const *engines_str = xgetenv("ARIFY_ENGINES", NULL);
    if (engines_str == NULL) {
        arify_errf("%s", "no engine specified");
        return -1;
    }

    size_t engines_len = strlen(engines_str) + 1;
    char *names = xmalloc(sizeof(char) * engines_len);
    memcpy(names, engines_str, engines_len);

    struct arify_engine head;
    for (struct arify_engine *engine = &head; ; *(names++) = '\0') {
        char *var_name = strchr(names, ':');
        if (var_name != NULL) {
            *(var_name++) = '\0';
        }
        engine = engine->next = xmalloc(sizeof(*engine));
        *engine = (struct arify_engine) {
            .lib_name = names,
            .var_name = var_name,
        };

        names = strchr(var_name == NULL ? names : var_name, ',');
        if (names == NULL) {
            break;
        }
    }
    ctx.current_engine = ctx.engines = head.next;
    return 0;
}

static int
config_frontend (void)
{
    char const *frontend_str = xgetenv("ARIFY_FRONTEND", "readline");
#ifdef ENABLE_ARIF_READLINE
    if (0 == strcmp("readline", frontend_str)) {
        ctx.frontend = &arify_frontend_readline;
        return 0;
    }
#endif  // defined(ENABLE_ARIF_READLINE)
    arify_errf("unsupported frontend '%s'", frontend_str);
    return -1;
}

static int
config_log_file (void)
{
    char const *log_file_path = xgetenv("ARIFY_LOG_FILE", NULL);
    if (log_file_path != NULL) {
        ctx.log_fd = open(log_file_path, O_WRONLY | O_CREAT | O_APPEND, 0600);
        ctx.pid    = getpid();
    }
    return 0;
}

static int
config_page_size (void)
{
    char const *page_size_str = xgetenv("ARIFY_PAGE_SIZE", "5");
    int page_size = atoi(page_size_str);
    if (page_size < 5 || page_size > ARIFY_MAX_PAGE_SIZE) {
        arify_errf("bad page size, expected [5, %d], got '%s'",
                ARIFY_MAX_PAGE_SIZE, page_size_str);
        return -1;
    }
    ctx.page_size = page_size;
    return 0;
}

static void
finalize (void)
{
    ctx.frontend->finalize(ctx.frontend_data);
    free((char *) ctx.engines->lib_name);
    finalize_engines(ctx.engines);
    arif_ctx_destroy(ctx.ctx);

    arify_debugf("%s", "finalized");
    close(ctx.log_fd);
}

static void
finalize_engines (
    struct arify_engine *engine
) {
    for (struct arify_engine *next; engine != NULL; engine = next) {
        if (engine->impl != NULL) {
            engine->impl->finalize(engine->data);
        }
        if (engine->handle != NULL) {
            dlclose(engine->handle);
        }

        next = engine->next;
        free(engine);
    }
}

static int
init_config (void)
{
    if (0 != config_log_file()) {
        return -1;
    }
    if (0 != config_frontend()) {
        return -1;
    }
    if (0 != config_page_size()) {
        return -1;
    }
    if (0 != config_engines()) {
        return -1;
    }
    return 0;
}

static void
initialize (void)
{
    if (0 != init_config()) {
        if (ctx.log_fd >= 0) {
            close(ctx.log_fd);
        }
        return;
    }
    atexit(finalize);

    struct arif_opts const opts = {
        .page_size = ctx.page_size,
    };
    ctx.ctx = arif_ctx_create(&opts);

    int result = ctx.frontend->init(ctx.ctx, &ctx.frontend_data);
    if (result != 0) {
        arify_errf("frontend init failed with return value %d", result);
        return;
    }

    arify_debugf("%s", "initialized");
}

static int
load_engine (
    struct arify_engine *engine
) {
    char const *lib_name = engine->lib_name;
    char const *var_name = engine->var_name;
    char *lib_tmp = NULL;
    char *var_tmp = NULL;

#define ARIFY_ENGINE_LIB(engine)  \
        ARIF_LIBDIR "/arif/" engine ARIF_SHLIB_SUFFIX
#define ARIFY_ENGINE_SYM(engine)  "arif_" engine "_engine"

    if (var_name == NULL) {
        size_t lib_name_len = sizeof(ARIFY_ENGINE_LIB("")) + strlen(lib_name);
        lib_tmp = xmalloc(sizeof(char) * lib_name_len);
        sprintf(lib_tmp, ARIFY_ENGINE_LIB("%s"), lib_name);

        size_t var_name_len = sizeof(ARIFY_ENGINE_SYM("")) + strlen(lib_name);
        var_tmp = xmalloc(sizeof(char) * var_name_len);
        sprintf(var_tmp, ARIFY_ENGINE_SYM("%s"), lib_name);

        lib_name = lib_tmp;
        var_name = var_tmp;
    }
    int result = -1;

    void *handle = dlopen(lib_name, RTLD_NOW);
    if (handle == NULL) {
        arify_errf("failed to load engine %s: %s", lib_name, dlerror());
        goto finish;
    }
    engine->handle = handle;

    struct arif_engine const *engine_impl = dlsym(handle, var_name);
    if (engine_impl == NULL) {
        arify_errf("failed to find symbol %s for engine %s: %s",
                var_name, lib_name, dlerror());
        goto finish;
    }

    int init_status = engine_impl->init(NULL, &engine->data);
    if (init_status != 0) {
        arify_errf("engine %s init failed with return value %d",
                lib_name, init_status);
        goto finish;
    }
    engine->impl = engine_impl;
    result = 0;

  finish:
    free(lib_tmp);
    free(var_tmp);
    return result;
}

void
arify_logf (
    char const *fmt,
    char const *level,
    ...
) {
    if (ctx.log_fd < 0) {
        return;
    }
    char log_buf[1024];
    char *buf = log_buf, *end = buf + sizeof(log_buf);

    time_t ts = time(NULL);
    struct tm const *tm = localtime(&ts);
    if (tm == NULL) {
        return;
    }
    buf += strftime(buf, end - buf, "[%T ", tm);
    buf += sprintf(buf, "%ld %s] ", (long) ctx.pid, level);

    va_list args;
    va_start(args, level);
    int len = vsnprintf(buf, end - buf, fmt, args);
    va_end(args);

    if (len >= end - buf) {
        len = end - buf;
        buf[len - 1] = '\n';
    }
    write(ctx.log_fd, log_buf, buf + len - log_buf);
}

struct arif_engine const *
arify_next_engine (
    void **engine_data_ptr
) {
    struct arify_engine      *engine      = ctx.current_engine;
    struct arif_engine const *engine_impl = NULL;
    void                     *engine_data = NULL;
    while (engine != NULL) {
        int result = load_engine(engine);

        engine_impl = engine->impl;
        engine_data = engine->data;
        engine      = engine->next;

        if (result == 0) {
            break;
        }
    }
    ctx.current_engine = engine;

    *engine_data_ptr = engine_data;
    return engine_impl;
}
