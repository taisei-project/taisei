Taisei Project - Build Options
==============================

This is intended for anyone looking to build Taisei for any of its supported operating systems, including Linux, macOS,
Windows, Emscripten, Switch, and others.

.. contents::

Checking Out Code
-----------------

First, you’ll need to checkout the repository. You can do that with the following:

.. code:: sh

   git clone --recurse-submodules https://github.com/taisei-project/taisei

The ``--recurse-submodules`` flag is absolutely necessary, or Taisei will not build, as it will be missing many of the
dependencies its needs to compile. If you accidentally omit it during checkout, you can fix it with:

.. code:: sh

   cd taisei/
   git submodule update --init --recursive

Dependencies
------------

Build-Time Dependencies
"""""""""""""""""""""""

- ``gcc`` or ``clang``
- meson >= 0.63.0
- Python >= 3.7
- `python-zstandard <https://github.com/indygreg/python-zstandard>`__ >= 0.11.1
- `python-docutils <https://pypi.org/project/docutils/>`__ (optional, for generating documentation)

Run-Time Dependencies
"""""""""""""""""""""

Required
''''''''

- OpenGL >= 3.3, or OpenGL ES >= 3.0
- SDL3 >= 3.2.0
- cglm >= 0.7.8
- libpng >= 1.5.0
- libwebpdecoder >= 0.5 or libwebp >= 0.5
- libzip >= 1.5.0 (>= 1.7.0 recommended)
- libzstd >= 1.4.0
- freetype2
- opusfile
- zlib

Optional
''''''''

- SPIRV-Cross >= 2019-03-22 (for OpenGL ES and SDL-GPU backends)
- libglslang (for OpenGL ES and SDL-GPU backends)
- `ANGLE <https://github.com/google/angle>`__ (useful for platforms with flaky/non-existent OpenGL support, such as
  Windows)
- GameMode headers (Linux only; for automatic `GameMode <https://github.com/FeralInteractive/gamemode>`__ integration)
- OpenSSL >= 1.1.0 or LibreSSL >= 2.7.0 (for a better SHA-256 implementation)

Built-In vs. System Dependencies
""""""""""""""""""""""""""""""""

Due to the wide array of platforms Taisei supports, we provide meson subprojects for its core dependencies using the
`Meson dependency wrap system <https://mesonbuild.com/Wrap-dependency-system-manual.html>`__. This is to facilitate
consistent build environments, including cross-builds, and for more esoteric platforms like Emscripten.

For convenience, Meson will detect which packages are missing from your system and use its wrap dependency system to
pull in what it can. Relying on this is *not* recommended in most circumstances, and you should instead rely on your
operating system’s package manager.

For consistency, we tend to release Taisei using exclusively built-in packages. However, you can also use system
dependencies as well. There’s a trade-off in consistency and reproducibility for speed and ease of use.

This is controlled through the ``--wrap-mode`` flag with Meson. (More on that later.)

Linux
'''''

On an Ubuntu or Debian-based distro, the following will install the mandatory tools for building Taisei.

.. code:: sh

   apt update
   apt install meson cmake build-essential

Beyond that, consult the *Dependencies* list above. Many distros package compile-time system dependencies with ``*-dev``
(i.e: ``libsdl2-dev``). Search with your distro’s package manager to install the correct libraries.

macOS
'''''

On macOS, you must install the Xcode Command Line Tools to build Taisei for the platform, as it contains headers and
tools for building native macOS apps.

.. code:: sh

   xcode-select --install

There are additional command line tools that you’ll need. You can acquire those by using `Homebrew
<https://brew.sh/>`__.

Follow the instructions for installing Homebrew, and then install the following tools:

.. code:: sh

   brew install meson cmake pkg-config docutils pygments

You can then install dependencies from the *Dependencies* list.

As of 2021-08-05, you should **not** install the following packages via Homebrew, as the versions available do not
compile against Taisei correctly. If you’re having mysterious errors, ensure that they’re not installed.

- ``spirv-tools``
- ``spirv-cross``

.. code:: sh

   brew remove spirv-tools spirv-cross

In addition, if you’re trying to compile on an older version of macOS (i.e: <10.12), SDL2 may not compile correctly on
Homebrew (as of 2019-02-19). Let Meson pull in the corrected version for you via subprojects.

You can also install `create-dmg <https://github.com/create-dmg/create-dmg>`__ for packaging ``.dmg`` files, which
enables some additional options such as positioning of icons in the ``.dmg``.

Windows
'''''''

Taisei uses `mstorsjo/llvm-mingw <https://github.com/mstorsjo/llvm-mingw>`__ to achieve cross-compiling for Windows.
Microsoft’s native C compiler toolchain simply does not support the things Taisei needs to compile correctly, including
fundamental things like `complex numbers <https://en.wikipedia.org/wiki/Complex_number>`__.

You can use ``llvm-mingw`` too, or you can check if your distro has any ``mingw64`` cross-compiler toolchains available
as well. That’s just the one that works for us.

Additionally, you can install `nsis <https://nsis.sourceforge.io/Main_Page>`__ (>= 3.0) for packaging Windows installer
``.exe`` files. (However, you can still package ``.zip`` files for Windows without it.)

On macOS, you’re probably better off using Docker and the `Docker container
<https://hub.docker.com/r/mstorsjo/llvm-mingw/>`__ that ``llvm-mingw`` provides, and installing ``nsis`` on top of it.

Another option for Windows-based computers is leveraging Windows 10’s `Windows Subsystem For Linux (WSL) Subsystem
<https://docs.microsoft.com/en-us/windows/wsl/install-win10>`__ to cross-compile to Windows using their Ubuntu image.
You can also potentially use a ``mingw64`` toolchain directly on Windows, however that isn’t supported or recommended,
as it’s generally more trouble than it’s worth.

Build Options
-------------

This is *not* an exhaustive list. You can see the full list of options using Meson in the ``taisei`` directory.

.. code:: sh

   cd taisei/
   meson configure

Setup
"""""

The first command you’ll need to run is ``setup``, which creates a directory (in this case, ``taisei/build/``). It
checks your system for various dependencies and required tools, which should take about a minute on most systems.

.. code:: sh

   # inside the taisei/ directory you cloned before
   meson setup build/

You can also have the ``setup`` command contain certain build options (seen below). The following are an *example* and
*not required* for getting Taisei building.

.. code:: sh

   # enables Developer Mode and debugging symbols
   meson setup build/ -Ddeveloper=true -Dbuildtype=debug

You can then apply more build options later using ``meson configure`` (as seen below). It will automatically reconfigure
your build environment with the new options without having to rebuild everything.

System/Vendored Dependencies (``--wrap-mode``)
""""""""""""""""""""""""""""""""""""""""""""""

See: `Meson Manual <https://mesonbuild.com/Wrap-dependency-system-manual.html>`__

- Default: ``default``
- Options: ``default``, ``nofallback``, ``forcefallback``, ...

This is a core Meson flag that does quite a few things. Not all of them will be covered here. Refer to the Meson
documentation linked above.

Generally, ``default`` will rely on system-installed libraries when available, and download missing dependencies when
necessary.

``forcefallback`` will force the use of wrapped dependencies whenever possible. Recommended for release builds.

``nofallback`` disallows the use of wrapped dependencies whenever possible, instead relying on system libraries. Useful
for CI.

.. code:: sh

   # forces in-repo dependencies
   meson configure build/ --wrap-mode=forcefallback
   # disables in-repo repositories
   meson configure build/ --wrap-mode=nofallback

Relative Directory Install (``-Dinstall_relocatable``)
""""""""""""""""""""""""""""""""""""""""""""""""""""""

- Default: ``auto``
- Options: ``auto``, ``enabled``, ``disabled``

This option enables a “relocatable” installation layout, where everything is confined to one directory and no full paths
are hardcoded into the executable.

The ``auto`` defaults to ``enabled`` when building for Windows, Emscripten, Switch, or macOS with
``install_macos_bundle`` enabled. Otherwise, it defaults to ``disabled``.

Note that you probably want to change the ``--prefix`` with this option enabled.

.. code:: sh

   meson configure build/ -Dinstall_relocatable=enabled

Install Prefix (``--prefix``)
"""""""""""""""""""""""""""""

- Default: ``/usr/local`` (usually; platform-dependent)

Specifies a path under which all game files are installed.

If ``install_relocatable`` is enabled, Taisei will be installed into the root of this directory, and thus you will
probably want to change it from the default value.

Otherwise, it’s not recommended to touch this option unless you know what you are doing.

This is a Meson built-in option; see `Meson Manual <https://mesonbuild.com/Builtin-options.html>`__ for more
information.

.. code:: sh

   meson setup --prefix=/path/goes/here build/

Package Data (``-Dpackage_data``)
"""""""""""""""""""""""""""""""""

- Default: ``auto``
- Options: ``auto``, ``enabled``, ``disabled``

If enabled, game assets will be packaged into a ``.zip`` archive. Otherwise, they will be installed into the filesystem
directly.

This option is not available for Emscripten.

Requires ``vfs_zip`` to be enabled as well.

.. code:: sh

   meson configure build/ -Dpackage_data=disabled

ZIP Package Loading (``-Dvfs_zip``)
"""""""""""""""""""""""""""""""""""

- Default: ``auto``
- Options: ``auto``, ``enabled``, ``disabled``

Controls whether Taisei can load game data (textures, shaders, etc.) from ``.zip`` files. Requires ``libzip``.

.. code:: sh

   meson configure build/ -Dvfs_zip=disabled

In-Game Developer Options (``-Ddeveloper``)
"""""""""""""""""""""""""""""""""""""""""""

- Default: ``false``
- Options: ``true``, ``false``

Enables various tools useful for developers and testers, such as cheats, stage menu, quick save/load, extra debugging
information, etc.

.. code:: sh

   meson configure build/ -Ddeveloper=true

Build Type (``-Dbuildtype``)
""""""""""""""""""""""""""""

- Default: ``release``
- Options: ``plain``, ``debug``, ``debugoptimized``, ``release``, ``minsize``, ``custom``

Sets the type of build. ``debug`` reduces optimizations and enables debugging symbols.

This is a Meson built-in option; see `Meson Manual <https://mesonbuild.com/Builtin-options.html>`__ for more
information.

.. code:: sh

   meson configure build/ -Dbuildtype=debug

Assertions (``-Db_ndebug``)
"""""""""""""""""""""""""""

- Default: ``if-release``
- Options: ``if-release``, ``true``, ``false``

The name of this flag is opposite of what you’d expect. Think of it as “Not Debugging”. It controls the ``NDEBUG``
declaration which is responsible for deactivating ``assert()`` macros.

Setting to ``false`` will *enable* assertions (good for debugging).

Keep ``true`` during release.

This is a Meson built-in option; see `Meson Manual <https://mesonbuild.com/Builtin-options.html>`__ for more
information.

.. code:: sh

   meson configure build/ -Db_ndebug=false

Strict Compiler Warnings (``-Dwerror``)
"""""""""""""""""""""""""""""""""""""""

- Default: ``false``
- Options: ``true``, ``false``

This option forces stricter checks against Taisei’s codebase to ensure code health, treating all ``Warnings`` as
``Errors`` in the code.

It’s highly recommended to **enable** this (i.e. set to ``true``) whenever developing for the engine. Sometimes it’s
overly pedantic, but much of the time it provides useful advice. For example, it can detect potential null pointer
dereferences that may not be obvious to the human eye.

This is a Meson built-in option; see `Meson Manual <https://mesonbuild.com/Builtin-options.html>`__ for more
information.

.. code:: sh

   meson configure build/ -Dwerror=true

Deprecation Warnings (``-Ddeprecation_warnings``)
"""""""""""""""""""""""""""""""""""""""""""""""""

- Default: ``default``
- Options: ``error``, ``no-error``, ``ignore``, ``default``

Sets deprecation warnings to either hard-fail (``error``), print as warnings but not trigger full errors if
``-Dwerror=true`` (``no-error``), and otherwise ignore them (``ignore``). ``default`` respects the ``-Dwerror`` setting.

Generally, ``no-error`` is the recommended default when using ``-Dwerror=true``.

.. code:: sh

   meson configure build/ -Ddeprecation_warnings=no-error


Debugging With Sanitizers (``-Db_sanitize``)
""""""""""""""""""""""""""""""""""""""""""""

This is useful for debugging memory management errors, leaks, and undefined behavior. However, there is some additional
setup required to use it.

.. code:: sh

   meson configure build/ -Db_sanitize=address,undefined

Depending on your platform, you may need to specify the specific library binary to use to launch ASan appropriately.
Using macOS as an example:

.. code:: sh

   export DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/12.0.5/lib/darwin/libclang_rt.asan_osx_dynamic.dylib

The ``../12.0.5/..`` in the path of ``DYLD_INSERT_LIBRARIES`` changes with each version of Xcode. If it fails to launch
for you, ensure that the version number is correct by browsing to the parent directory of ``../clang``.

Then, you can launch Taisei’s binary from the command line (using macOS as an example):

This is a Meson built-in option; see `Meson Manual <https://mesonbuild.com/Builtin-options.html>`__ for more
information.

.. code:: sh

   /path/to/Taisei.app/Contents/MacOS/Taisei

Further reading: `Sanitizers <https://github.com/google/sanitizers/wiki>`__

Link-Time Optimizations (``-Db_lto``)
"""""""""""""""""""""""""""""""""""""

- Default: ``true``
- Options: ``true``, ``false``

Link-time optimizations (LTO) increase build times, but also increase performance. For quicker build times during
development, you can disable it. For release builds, this should be kept ``true``.

See: `Interprocedural Optimization <https://en.wikipedia.org/wiki/Interprocedural_optimization#WPO_and_LTO>`__

This is a Meson built-in option; see `Meson Manual <https://mesonbuild.com/Builtin-options.html>`__ for more
information.

.. code:: sh

   meson configure build/ -Db_lto=false

Binary Striping (``-Dstrip``)
"""""""""""""""""""""""""""""

- Default: ``true``
- Options: ``true``, ``false``

This option prevents stripping of the `taisei` binary, providing a marginally faster build time.

Keep this ``true`` during releases, but ``false`` during development, as it will strip out useful debugging symbols.

.. code:: sh

   meson configure build/ -Db_strip=false

Rendering
"""""""""

Backends (``-Dr_*``)
''''''''''''''''''''

- Default: ``auto``
- Options: ``auto``, ``enabled``, ``disabled``

Enable or disable the various renderer backends for Taisei.

``-Dshader_transpiler`` is required for when OpenGL ES is used.

.. code:: sh

   # for GL 3.3 (default)
   meson configure build/ -Dr_gl33=enabled
   # for GL ES 3.0
   meson configure build/ -Dr_gles30=enabled
   # for SDL-GPU (Vulkan, Metal, D3D12)
   meson configure build/ -Dr_sdlgpu=enabled
   # No-op backend (nothing displayed).
   # Disabling this will break the replay-verification mode.
   meson configure build/ -Dr_null=enabled

Default Renderer (``-Dr_default``)
''''''''''''''''''''''''''''''''''

- Default: ``auto``
- Options: ``auto``, ``gl33``, ``gles30``, ``null``

Sets the default renderer to use when Taisei launches.

When set to ``auto``, defaults to the first enabled backend in this order: ``gl33``, ``gles30``.

The chosen backend must not be disabled.

.. code:: sh

   # for GL 3.3 (default)
   meson configure build/ -Dr_default=gl33
   # for GL ES 3.0
   meson configure build/ -Dr_default=gles30

You can switch the renderer using the ``--renderer`` flag with the ``taisei`` binary, like this: ``taisei --renderer
gles30``.

Shader Transpiler (``-Dshader_transpiler``)
'''''''''''''''''''''''''''''''''''''''''''

- Default: ``auto``
- Options: ``auto``, ``enabled``, ``disabled``

For using OpenGL ES or SDL-GPU, the shader transpiler is necessary for converting Taisei’s shaders to a format usable by
that driver.

Requires ``glslang`` and ``SPIRV-cross``.

Note that for Emscripten and Switch platforms, the translation is performed offline, and this option is not available.

.. code:: sh

   meson configure build/ -Dshader_transpiler=enabled

ANGLE
"""""

Building ANGLE (Optional)
'''''''''''''''''''''''''

`ANGLE <https://github.com/google/angle>`__ is Google’s graphics translation layer, intended for Chromium. Taisei
packages it with Windows builds to workaround some bugs and performance issues with many Windows OpenGL drivers, and it
can be optionally packaged as an experimental Metal renderer for macOS.

You need to read `this guide <https://github.com/google/angle/blob/master/doc/DevSetup.md>`__ and set up Google’s custom
build system to get things going. However, the below commands might help you compiling what you need from it when you
have that all set up.

.. code:: sh

   cd angle
   python ./scripts/bootstrap.py
   gclient sync
   gn gen out/x64 --args='is_debug=false dcheck_always_on=false target_cpu="x64"'
   ninja -C out/x64 libEGL libGLESv2

It will output two files to ``angle/out/x64``:

- ``libEGL.(*)``
- ``libGLESv2.(*)``

The file extension can be ``.dll`` for Windows, ``.dylib`` for macOS, and ``.so`` for Linux.

Using ``-Dinstall_angle`` and ``-Dangle_lib*`` (see below), Meson will copy those files over into the package itself
when running the packaging steps.

ANGLE Library Paths (``-Dangle_lib*``)
''''''''''''''''''''''''''''''''''''''

- Default: ``(null)``
- Options: ``/path/to/libGLESv2.{dll,dylib,so}``/``path/to/libEGL.{dll,dylib,so}``

``-Dangle_libgles`` and ``-Dangle_libegl`` provide the full paths to the ANGLE libraries necessary for that engine.

Generally, both need to be supplied at the same time.

.. code:: sh

   # for macOS
   meson configure build/ -Dangle_libgles=/path/to/libGLESv2.dylib -Dangle_libegl=/path/to/libEGL.dylib
   # for Linux
   meson configure build/ -Dangle_libgles=/path/to/libGLESv2.so -Dangle_libegl=/path/to/libEGL.so
   # for Windows
   meson configure build/ -Dangle_libgles=/path/to/libGLESv2.dll -Dangle_libegl=/path/to/libEGL.dll

Install ANGLE (``-Dinstall_angle``)
'''''''''''''''''''''''''''''''''''

- Default: ``false``
- Options: ``true``, ``false``

Installs the ANGLE libraries supplied above through ``-Dangle_lib*``.

Generally recommended when packaging ANGLE for distribution.

.. code:: sh

   meson configure build/ -Dinstall_angle=true

