/**
 * arif/include/arif_defs.h - ARIF common macros
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

#ifdef HAVE_VAR_ATTRIBUTE_UNUSED
#  define ARIF_UNUSED  __attribute__((unused))
#else
#  define ARIF_UNUSED
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
