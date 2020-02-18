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

Follow the instructions for installing Homebrew, and then install the following 
tools:

::

    brew install meson cmake pkg-config docutils


The following dependencies are technically optional, and can be pulled in at
build-time, but you're better off installing them yourself to reduce compile
times:

::

    brew install freetype2 libzip opusfile libvorbis webp sdl2 


You'll also want to set the following environment variables on your shell's
.zshrc or .bash_profile:

::

   export PKG_CONFIG_PATH="/usr/local/opt/openssl@1.1/lib/pkgconfig" 


As of 2020-02-18, you should **not** install the following packages via
Homebrew, as the versions available do not compile against Taisei correctly. 
If you're having mysterious errors, ensure that they're not installed.

* ``spirv-tools``
* ``spirv-cross``
* ``sdl2_mixer``

Remove them with:

::

    brew remove spirv-tools spirv-cross sdl2_mixer


Taisei-compatible versions will be pulled in at compile time.


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

