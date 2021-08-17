Taisei Project - Build/Maintainer FAQ
=====================================

This is intended for anyone looking to build or package Taisei Project
for any of its supported operating systems, including Linux, macOS, Windows,
Emscripten, Switch, and others.

.. contents::

Dependencies
------------

Taisei's general dependencies are as follows:

-  C (C11) compiler (``gcc``, ``clang``, etc)
-  Python >= 3.5
-  meson >= 0.53.0 (0.56.2 recommended)
-  ninja >= 1.10.0
-  docutils

You can optionally install `other dependencies <#platform-specific-tips>`__,
but the build system will pull in everything else you might need otherwise.

Linux (Debian/Ubuntu)
"""""""""""""""""""""

On an apt-based system (Debian/Ubuntu), ensure you have build dependencies
installed:

.. code:: sh

   apt-get install meson cmake build-essential
   apt-get install libsdl2-dev libsdl2-mixer-dev libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev

If your distribution of Linux uses Wayland as its default window server, ensure
that Wayland deps are installed:

.. code:: sh

   apt-get install libwayland-dev

For packaging, your best bet is ``.zip``. Invoke ``ninja`` to package a
``.zip``:

.. code:: sh

   ninja zip -C build/

macOS
"""""

On macOS, you need to begin with installing the Xcode Command Line Tools:

.. code:: sh

   xcode-select --install

There are additional command line tools that you'll need. You can acquire those
by using `Homebrew <https://brew.sh/>`__.

Follow the instructions for installing Homebrew, and then install the following
tools:

.. code:: sh

   brew install meson cmake pkg-config docutils imagemagick pygments

The following dependencies are technically optional, and can be pulled in at
build-time, but you're better off installing them yourself to reduce compile
times:

.. code:: sh

   brew install freetype2 libzip opusfile libvorbis webp sdl2

As of 2021-08-05, you should **not** install the following packages via
Homebrew, as the versions available do not compile against Taisei correctly.
If you're having mysterious errors, ensure that they're not installed.

* ``spirv-tools``
* ``spirv-cross``
* ``sdl2_mixer``

Remove them with:

.. code:: sh

   brew remove spirv-tools spirv-cross sdl2_mixer

Taisei-compatible versions are bundled and will be pulled in at compile time.

In addition, if you're trying to compile on an older version of macOS
(i.e: <10.12), SDL2 may not compile correctly on Homebrew (as of 2019-02-19).
Let ``meson`` pull in the corrected version for you via subprojects.

**NOTE:** While Homebrew's optional dependencies greatly improve compile times,
if you can't remove packages that give you errors from your system for whatever
reason, you can force ``meson`` to use its built-in subprojects by using the
following option:


Optionally, if you're on macOS and compiling for macOS, you can to install
`create-dmg <https://github.com/create-dmg/create-dmg>`__, which will allow
you to have nicer-looking macOS ``.dmg`` files for distribution:

.. code:: sh

   brew install create-dmg

You can create a ``.dmg`` on either Linux or macOS (although with ``create-dmg``
on macOS, the macOS-produced ``.dmg`` will look nicer):

.. code:: sh

   ninja dmg -C build/

Windows
"""""""

While the game itself officially supports Windows, building the project
directly on Windows is a bit difficult to set up due to the radically different
tooling required for a native Windows build environment.

However, you can still compile on a Windows-based computer by leveraging Windows
10's
`Windows For Linux (WSL) Subsystem <https://docs.microsoft.com/en-us/windows/wsl/install-win10>`__
to cross-compile to Windows. Ironically enough, compiling for Windows on Linux
ends up being easier and more consistent than trying to compile with Windows's
native toolset.

Taisei uses `mstorsjo/llvm-mingw <https://github.com/mstorsjo/llvm-mingw>`__ to
achieve cross-compiling on Windows. We also have a ``meson`` machine file
located at ``misc/ci/windows-llvm_mingw-x86_64-build-test-ci.ini`` to go with
that toolchain. In general, you'll need the following tools for compiling Taisei
for Windows on Linux:

- ``llvm-mingw``
- `nsis <https://nsis.sourceforge.io/Main_Page>`__ >= 3.0

On macOS, you're probably better off using Docker and the
`Docker container <https://hub.docker.com/r/mstorsjo/llvm-mingw/>`__ that
``llvm-mingw`` provides, and installing ``nsis`` on top of it. Refer to
``misc/ci/Dockerfile.windows`` for more insight.

Additionally, on Windows, you'll need to make sure you have *ANGLE support*
enabled, as previously mentioned.

Checking Out Code
-----------------

First, you'll need to checkout the repository. You can do that with the
following:

.. code:: sh

   git clone https://github.com/taisei-project/taisei.git
   cd taisei/
   git submodule update --init --recursive

The ``git submodule update --init --recursive`` line is absolutely necessary,
or Taisei will not build, as it will be missing many of the dependencies its
needs to compile.

Build Options
-------------

Force Vendored Dependencies (``--wrap-mode``)
"""""""""""""""""""""""""""""""""""""""""""""

See: `Meson Wrap Dependency Manual <https://mesonbuild.com/Wrap-dependency-system-manual.html>`__

* Default: ``default``
* Options: ``default``, ``nofallback``, ``forcefallback``, ...

This is a ``meson`` flag that does quite a few things. Not all of them will be
covered here. Refer to the documentation linked above.

Generally, ``default`` will rely on system-installed libraries when available,
and fallback to vendored in-repository dependencies when necessary.

``forcefallback`` will heavily encourage the use of in-repository dependencies
whenever possible. Recommended for release builds.

``nofallback`` discourages the use of in-repository dependencies whenever
possible, instead relying on system libraries. Useful for CI.

.. code:: sh

   # for release builds
   meson configure build/ --wrap-mode=forcefallback
   # useful for testing/CI
   meson configure build/ --wrap-mode=nofallback

Faster Builds (``-Db_lto``/``-Dstrip``)
"""""""""""""""""""""""""""""""""""""""

* Defaults: ``false``
* Options: ``true``, ``false``

These options prevent stripping of the binaries, leading to faster build times
and keeping debugging symbols in place. There is a theoretical performance hit
with these options enabled, but it can help with building during development.

.. code:: sh

   meson configure build/ -Db_lto=false -Dstrip=false

Developer Mode (``-Ddeveloper``)
""""""""""""""""""""""""""""""""

* Default: ``false``
* Options: ``true``, ``false``

For testing actual gameplay, you can set this option and it will enable cheats
and other 'fast-forward' options by the pressing keys defined in
``src/config.h``.

.. code:: sh

   meson configure build/ -Ddeveloper=true

Build Type (``-Dbuildtype``)
""""""""""""""""""""""""""""

* Default: ``release``
* Options: ``release``, ``debug``

Sets the type of build. ``debug`` enables several additional debugging features.
Useful for development.

.. code:: sh

   meson configure build/ -Dbuildtype=debug

Debugging (``-Db_ndebug``)
""""""""""""""""""""""""""

* Default: ``true``
* Options: ``true``, ``false``

The name of this flag is opposite of what you'd expect. Think of it as "Not
Debugging". If ``true``, it is *not* a debug build. If ``false``, it *is* a
debugging build.

.. code:: sh

   meson configure build/ -Db_ndebug=false

Stack Trace Debugging (``-Db_sanitize``)
""""""""""""""""""""""""""""""""""""""""

This is useful for debugging crashes in the game. It uses
`AddressSanitizer <https://github.com/google/sanitizers/wiki/AddressSanitizer>`__:

.. code:: sh

   meson configure build/ -Db_sanitize=address,undefined

Depending on your platform, you may need to specify the specific library binary
to use to launch ASan appropriately. Using macOS as an example:

.. code:: sh

   export DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/12.0.0/lib/darwin/libclang_rt.asan_osx_dynamic.dylib

The ``../12.0.0/..`` in the path of ``DYLD_INSERT_LIBRARIES`` changes with each
version of XCode. If it fails to launch for you, ensure that the version number
is correct by browsing to the parent directory of ``../clang``.

Then, you can launch Taisei's binary from the command line (using macOS as an
example):

.. code:: sh

   /path/to/Taisei.app/Contents/MacOS/Taisei

Install Prefix (``--prefix``)
"""""""""""""""""""""""""""""

* Default: ``/usr/local``

``--prefix`` installs the Taisei binary and content files to a path of your
choice.

.. code:: sh

   meson setup --prefix=/path/goes/here -C build/

Rendering
"""""""""

Backends (``-Dr_gl*``)
''''''''''''''''''''''

* Default: ``false``
* Options: ``true``, ``false``

Enable or disable the various renderer backends for Taisei.

.. code:: sh

   # for GL 3.3 (default)
   meson configure build/ -Dr_gl33=true
   # for GL ES 3.0
   meson configure build/ -Dr_gles30=true
   # for GL ES 2.0
   meson configure build/ -Dr_gles20=true

Note that GL ES 2.0 requires a few extensions to be present on your system
to function correctly, most notably:

- ``OES_depth_texture`` or ``GL_ANGLE_depth_texture``
- ``OES_standard_derivatives``
- ``OES_vertex_array_object``
- ``EXT_frag_depth``
- ``EXT_instanced_arrays`` or ``ANGLE_instanced_arrays`` or
  ``NV_instanced_arrays``

Default Renderer (``-Dr_default``)
''''''''''''''''''''''''''''''''''

* Default: ``gl33``
* Options: ``gl33``, ``gles30``, ``gles20``, ``null``

.. code:: sh

   # for GL 3.3 (default)
   meson configure build/ -Dr_default=gl33
   # for GL ES 3.0
   meson configure build/ -Dr_default=gles30
   # for GL ES 2.0
   meson configure build/ -Dr_default=gles20

ANGLE
"""""

Shader Transpiler (``-Dshader_transpiler``)
'''''''''''''''''''''''''''''''''''''''''''

* Default: ``false``
* Options: ``true``, ``false``

For using ANGLE, the shader transpiler is necessary for converting Taisei's
shaders to a format usable by that driver.

.. code:: sh

   meson configure build/ -Dshader_transpiler=true

ANGLE Library Paths (``-Dangle_lib*``)
''''''''''''''''''''''''''''''''''''''

* Default: ``(null)``
* Options: ``/path/to/libGLESv2.{dll,dylib,so}``/``path/to/libEGL.{dll,dylib,so}``

``-Dangle_libgles`` and ``-Dangle_libegl`` provide the full paths to the ANGLE
libraries necessary for that engine.

Generally, both need to be supplied at the same time.

.. code:: sh

   # for Linux
   meson configure build/ -Dangle_libgles=/path/to/libGLESv2.dylib -Dangle_libegl=/path/to/libEGL.dylib
   # for macOS
   meson configure build/ -Dangle_libgles=/path/to/libGLESv2.so -Dangle_libegl=/path/to/libEGL.so
   # for Windows
   meson configure build/ -Dangle_libgles=/path/to/libGLESv2.dll -Dangle_libegl=/path/to/libEGL.dll

Install ANGLE (``-Dinstall_angle``)
'''''''''''''''''''''''''''''''''''

* Default: ``false``
* Options: ``true``, ``false``

Installs the ANGLE libraries supplied above through ``-Dangle_lib*``.

Generally recommended when packaging ANGLE for distribution.

.. code:: sh

   meson configure build/ -Dinstall_angle=true

Building ANGLE (Windows/macOS)
------------------------------

ANGLE is Google's graphics translation layer, intended for for Chromium. Taisei
packages it with Windows builds to workaround some bugs and performance issues
with many Windows OpenGL drivers, and it can be optionally packaged as as an
experimental Metal renderer for macOS.

You'll need to check out
`ANGLE <https://github.com/google/angle>`__ and build it first. Refer to their
documentation on how to do that, but generally:

.. code:: sh

   cd angle
   python ./scripts/bootstrap.py
   gclient sync
   gn gen out/x64 --args='is_debug=false dcheck_always_on=false target_cpu="x64"'
   ninja -C out/x64 libEGL libGLESv2

It will output two files to ``angle/out/x64``:

* ``libEGL.(*)``
* ``libGLESv2.(*)``

The file extension can be ``.dll`` for Windows, ``.dylib`` for macOS,
and ``.so`` for Linux.

Using ``-Dinstall_angle`` (referenced above), ``meson`` will copy those files
over into the package itself when running the packaging steps.

Packaging
---------

Building Examples
"""""""""""""""""

Linux
'''''

macOS
'''''

Windows
'''''''
