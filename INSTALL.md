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

  Other software (optional):
  - DejaGnu (for running tests)
  - Rime IME core library: <https://github.com/rime/librime>


Build and Install
-----------------

  Select a build directory and generate configuration scripts:

    $ mkdir build && cd build
    $ autoreconf ..

  To list all available configuration options, run:

    $ ../configure --help

  Notable options:
  - `--disable-arify`
    * Do not build the `arify` program.
  - `--disable-arif-readline`
    * Build the ARIF library without GNU Readline features.
  - `--enable-rl-loop`
    * Build the `rl-loop` program.
  - `--enable-arif-rime`
    * Build the example Rime IME integration.

  If a dependency is installed in a custom location, it could be specified
  with `--with-<lib>=<pkgconfdir>`, where `<lib>` is the library name,
  and `<pkgconfdir>` is the directory holding its pkg-config file.

  An example configuration:

    $ ../configure --prefix="$HOME/.local"  \
    >         --enable-arif-rime  \
    >         --with-rime="$HOME/.local/lib/pkgconfig"  \
    >         CFLAGS='-O2'

  After configuration, Build the binaries:

    $ make

  Install:

    $ make install

  Uninstall:

    $ make uninstall
