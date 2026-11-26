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
    * Confirmed to be fully functional on GNU/Linux, FreeBSD and NetBSD.
  - GNU Readline (optional)

  Build tools:
  - GNU Autotools (Autoconf, Automake, Libtool)
  - pkg-config
  - POSIX-compliant `make`
  - C compiler with ISO C99 support
    * Requires `__attribute__((constructor))` for `arify` to work.

  Other software (optional):
  - DejaGnu (for running tests)
  - Rime IME core library: <https://github.com/rime/librime>
  - libpinyin: <https://github.com/libpinyin/libpinyin>


Build and Install
-----------------

  Select a build directory and generate configuration scripts:

    $ mkdir build && cd build
    $ autoreconf -i ..

  To list all available configuration options, run:

    $ ../configure --help

  Notable options:
  - `--disable-arify`
    * Do not build the `arify` program and the preload library.
  - `--disable-arif-readline`
    * Build the ARIF library without GNU Readline features.
  - `--enable-rlexec`
    * Build the `rlexec` program.
  - `--enable-arif-rime`
    * Build the example Rime IME integration.
  - `--enable-arif-pinyin`
    * Build the example libpinyin integration.

  If a dependency is installed in a custom location, it could be specified
  with `--with-<lib>=<pkgconfdir>`, where `<lib>` is the library name,
  and `<pkgconfdir>` is the directory holding its pkg-config file.

  An example configuration:

    $ ../configure --prefix="$HOME/.local"  \
    >         --enable-arif-rime  \
    >         --with-rime="$HOME/.local/lib/pkgconfig"  \
    >         CFLAGS='-O2'

  After configuration, build the binaries and run tests:

    $ make
    $ make check

  Install:

    $ make install

  Uninstall:

    $ make uninstall
