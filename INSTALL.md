<!--
  Copyright (C) 2023  CismonX <admin@cismon.net>
  
  Copying and distribution of this file, with or without modification, are
  permitted in any medium without royalty, provided the copyright notice and
  this notice are preserved. This file is offered as-is, without any warranty.
-->


Prerequisites
-------------

  Runtime requirements:
  - POSIX.1-2001 compliant operating system
  - GNU Readline (optional)

  Build tools:
  - GNU Autotools (Autoconf, Automake, Libtool, Autoconf Archive)
  - pkg-config
  - POSIX-compliant make
  - C compiler with ISO C99 support
  - GNU Texinfo (optional, for building the user manual)

  Other softwares (optional):
  - DejaGnu (for running tests)
  - Rime IME core library: <https://github.com/rime/librime>


Build and Install
-----------------

  ARIF can be built and installed from source using the GNU build system.

  Notable build options:
  - `--with-readline[=PREFIX]` (default), `--without-readline`
    * Whether to build the ARIF library with GNU Readline features.
  - `--enable-arify` (default), `--disable-arify`
    * Whether to build the `arify` command-line tool.
  - `--with-rime[=PREFIX]`, `--without-rime` (default)
    * Whether to build the example Rime IME integration.

  Example shell script:

    autoreconf -i
    mkdir build && cd build
    ../configure --prefix=${HOME}/.local
    make
    make install
