Taisei Project - Compiling FAQ
==============================

.. contents::

Introduction
------------

This document contains some important tips on compiling and developing for 
specific platforms. 

OS-specific Tips
----------------

macOS (manual)
~~~~~~~~~~~~~~

On macOS, you need to begin with installing the Xcode Command Line Tools:

::

    xcode-select --install

There are additional command line tools that you'll need. You can acquire those
by using `Homebrew <https://brew.sh/>`__. 

Follow the instructions for installing that, and then install the following 
tools:

::

    brew install meson cmake pkg-config docutils


The following dependencies are technically optional, and can be pulled in at
build-time, but you're better off installing them yourself to reduce compile
times:

::

    brew install freetype2 libzip opusfile libvorbis webp sdl2 spirv-tools

NOTE: do *NOT* install the Homebrew version of `sdl2_mixer`. It lacks OPUS
support and Taisei will not play any sounds. Let `meson` pull in the corrected
version.

You'll also want to set the following environment variables on your shell's
.zshrc or .bash_profile:

::

   export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig" 

macOS (automatic)
~~~~~~~~~~~~~~~~~

To do the above steps automatically, run `scripts/macos-deps-install.sh`.


Linux (Debian/Ubuntu)
~~~~~~~~~~~~~~~~~~~~~

On an apt-based system (Debian/Ubuntu), ensure you have build dependencies
installed:

::

    apt-get install meson cmake build-essential libsdl2-dev libsdl2-mixer-dev 
    libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev libwayland-dev

