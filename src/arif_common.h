/**
 * arif/src/arif_common.h - ARIF common macros and helper functions
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

#ifndef ARIF_COMMON_H_
#define ARIF_COMMON_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define ARIF_INTERNAL
#define ARIF_UNUSED
#ifdef __has_attribute
#  if __has_attribute(visibility)
#    undef  ARIF_INTERNAL
#    define ARIF_INTERNAL  __attribute__((visibility("hidden")))
#  endif
#  if __has_attribute(unused)
#    undef  ARIF_UNUSED
#    define ARIF_UNUSED  __attribute__((unused))
#  endif
#  if !defined(ARIF_CTOR) && __has_attribute(constructor)
#    define ARIF_CTOR  __attribute__((constructor))
#  endif
#endif
#define ARIF_UNUSED_ARG(name)  name##_unused_ ARIF_UNUSED

#if defined(__APPLE__)
#  define ARIF_SHLIB_SUFFIX ".dylib"
#elif defined(__CYGWIN__) || defined(__MINGW32__)
#  define ARIF_SHLIB_SUFFIX ".dll"
#else
#  define ARIF_SHLIB_SUFFIX ".so"
#endif

#ifndef ARIF_LIBDIR
#  define ARIF_LIBDIR  "/usr/local/lib"
#endif

#ifdef __FILE_NAME__
#  define FILE_NAME_  __FILE_NAME__
#else
#  ifdef ARIF_SRCDIR
#    define FILE_NAME_OFFSET_  sizeof(ARIF_SRCDIR)
#  else
#    define FILE_NAME_OFFSET_  0
#  endif
#  define FILE_NAME_  ( __FILE__ + FILE_NAME_OFFSET_ )
#endif

#define ARIF_LOGF_(fmt, ...)  \
    "%s:%d: " fmt "\n", FILE_NAME_, __LINE__, __VA_ARGS__
#define arif_logf(fmt, ...)  fprintf(stderr, ARIF_LOGF_(fmt, __VA_ARGS__))

#define xassert(e)  \
    if (!(e)) {     \
        arif_logf("assertion (%s) failed", #e);  \
        abort();    \
    }

static inline void *
xmalloc (
    size_t n
) {
    void *p = malloc(n);
    xassert(p != NULL);
    return p;
}

static inline void *
xrealloc (
    void   *p,
    size_t  n
) {
    p = realloc(p, n);
    xassert(p != NULL);
    return p;
}

static inline char const *
xgetenv (
    char const *name,
    char const *default_val
) {
    char const *val = getenv(name);
    if (val == NULL || val[0] == '\0') {
        val = default_val;
    }
    return val;
}

static inline int
xasprintf (
    char       **strp,
    char const  *fmt,
    ...
) {
    va_list args;
    va_start(args, fmt);

#if defined(__FreeBSD__) || (defined(__linux__) && defined(_GNU_SOURCE))
    int len = vasprintf(strp, fmt, args);
    xassert(len >= 0);
#else
    int len = vsnprintf(NULL, 0, fmt, args);
    xassert(len >= 0);

    *strp = xmalloc(len + 1);
    va_end(args);
    va_start(args, fmt);
    xassert(len == vsprintf(*strp, fmt, args));
#endif

    va_end(args);
    return len;
}

#endif  // !defined(ARIF_COMMON_H_)
