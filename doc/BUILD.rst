Taisei Project - Build/Maintainer FAQ
=====================================

This is intended for anyone looking to build or package Taisei Project
for any of its supported operating systems, including Linux, macOS, Windows,
Emscripten, Switch, and others.

.. contents::

Dependencies
------------

Taisei's mandatory dependencies are as follows:

-  C (C11) compiler (``gcc``, ``clang``, etc)
-  Python >= 3.5
-  meson >= 0.56.2 (recommended)
-  ninja >= 1.10.0
-  docutils

Built-In vs. System Dependencies
""""""""""""""""""""""""""""""""

Due to the wide array of platforms Taisei supports, the project packages many of
its core dependencies inside the project using the
`Meson dependency wrap system <https://mesonbuild.com/Wrap-dependency-system-manual.html>`__.
This is used to pin specific versions of libraries such as `SDL2` to known-good
versions, as well as assist with packaging and distribution on older and/or more
esoteric systems (i.e: Emscripten).

However, recompiling every single dependency Taisei needs every time you want to
rebuild the project is not ideal. You can still use system dependencies
installed through your OS's package manager (`apt`, `yum`, `brew`, etc) for
development, local testing, or simply building the game for your own system.

For convenience, ``meson`` will detect which packages are missing from your
system and use its wrap dependency system to pull in what it can. The trade-off
is that this can result in longer build times.

For releases, however, you should rely *exclusively* on the ``meson`` wrap
dependencies with the Taisei repository, for consistency and

This is controlled through the ``--wrap-mode`` flag with ``meson``. (More
on that later.)

In summary:

- Development = system-install dependencies are fine (`apt`, `brew`, etc)
- Release = ``meson`` wrap dependencies only

System Dependencies
"""""""""""""""""""

Linux
'''''

(The following example is specific to Ubuntu or Debian, but you can substitute
`apt` for the package manager for your flavour of Linux.)

The following will install the mandatory tools for building Taisei.

.. code:: sh

   apt update
   apt install meson cmake build-essential


The dependencies below are technically optional, as mentioned above in
``Built-In Vs. System Dependencies``.

Keep in mind that certain packages may have different names on different
systems. Ensure you are installing the ``dev`` versions of those packages.

.. code:: sh

   apt install libsdl2-dev libsdl2-mixer-dev libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev

If your distribution of Linux uses Wayland as its default window server, ensure
that Wayland deps are installed:

.. code:: sh

   apt install libwayland-dev

macOS
"""""

On macOS, you must install the Xcode Command Line Tools to build Taisei for
the platform, as it contains headers and tools that aren't included in the FOSS
versions found in ``brew``.

.. code:: sh

   xcode-select --install

There are additional command line tools that you'll need. You can acquire those
by using `Homebrew <https://brew.sh/>`__.

Follow the instructions for installing Homebrew, and then install the following
tools:

.. code:: sh

   brew install meson cmake pkg-config docutils imagemagick pygments

The following dependencies are technically optional, and can be pulled in at
build-time, but they will reduce compile times during development, so it's
recommended to install them.

.. code:: sh

   brew install freetype2 libzip opusfile libvorbis webp sdl2

Optionally, if you're on macOS and compiling for macOS, you can to install
`create-dmg <https://github.com/create-dmg/create-dmg>`__, which will allow
you to have nicer-looking macOS ``.dmg`` files for distribution:

.. code:: sh

   brew install create-dmg

As of 2021-08-05, you should **not** install the following packages via
Homebrew, as the versions available do not compile against Taisei correctly.
If you're having mysterious errors, ensure that they're not installed.

-  ``spirv-tools``
-  ``spirv-cross``
-  ``sdl2_mixer``

.. code:: sh

   brew remove spirv-tools spirv-cross sdl2_mixer

In addition, if you're trying to compile on an older version of macOS
(i.e: <10.12), SDL2 may not compile correctly on Homebrew (as of 2019-02-19).
Let ``meson`` pull in the corrected version for you via subprojects.

**NOTE:** While Homebrew's optional dependencies greatly improve compile times,
if you can't remove packages that give you errors from your system for whatever
reason, you can force ``meson`` to use its built-in subprojects by using
``--wrap-mode`` (more on that later).

Windows
"""""""

Taisei uses `mstorsjo/llvm-mingw <https://github.com/mstorsjo/llvm-mingw>`__ to
achieve cross-compiling on Windows. Cross-compiling for Windows ends up being
easier to maintain and more consistent than attempting to use Microsoft's native
toolchain.

On Linux, you'll need the following tools for cross-compiling Taisei for Windows
on Linux:

- ``llvm-mingw``
- `nsis <https://nsis.sourceforge.io/Main_Page>`__ >= 3.0

On macOS, you're probably better off using Docker and the
`Docker container <https://hub.docker.com/r/mstorsjo/llvm-mingw/>`__ that
``llvm-mingw`` provides, and installing ``nsis`` on top of it.

Another options for Windows-based computers is leveraging Windows
10's
`Windows For Linux (WSL) Subsystem <https://docs.microsoft.com/en-us/windows/wsl/install-win10>`__
to cross-compile to Windows using their Ubuntu image.

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

Setup
"""""

The first command you'll need to run is ``setup``, which creates a directory
(in this case, ``taisei/build/``). It checks your system for various
dependencies and required tools, which should take about a minute on most
systems.

.. code:: sh

   # inside the taisei/ directory you cloned before
   meson setup -C build/

You can also have the ``setup`` command contain certain build options (seen
below). The following are an *example* and *not required* for getting Taisei
building.

.. code:: sh

   # enables Developer Mode and debugging symbols
   meson setup -C build/ -Ddeveloper=true -Dbuildtype=debug

You can then apply more build options later using ``meson configure`` (as seen
below). It will automatically reconfigure your build environment with the new
options without having to rebuild everything.

System/Vendored Dependencies (``--wrap-mode``)
""""""""""""""""""""""""""""""""""""""""""""""

See: `Meson Manual <https://mesonbuild.com/Wrap-dependency-system-manual.html>`__

* Default: ``default``
* Options: ``default``, ``nofallback``, ``forcefallback``, ...

This is a core ``meson`` flag that does quite a few things. Not all of them will
be covered here. Refer to the ``meson`` documentation linked above.

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

Relative Directory Install (``-Dinstall_relative``)
"""""""""""""""""""""""""""""""""""""""""""""""""""

* Default: ``true`` or ``false`` (platform-dependent)

``-Dinstall_relative`` is a special option that changes depending on the
platform build target.

It is set to ``true`` when building for macOS, Windows, Emscripten, and Switch.

It is set to ``false`` when building for Linux.

Install Prefix (``--prefix``)
"""""""""""""""""""""""""""""

* Default: ``/usr/local``

``--prefix`` installs the Taisei binary and content files to a path of your
choice on your filesystem.

On Linux without ``-Dinstall_relative`` enabled (i.e: ``false``), it should be
set to ``/usr/local``.

On other platforms, it will install all Taisei game files to the directory of
your choice.

.. code:: sh

   meson setup --prefix=/path/goes/here -C build/

Package Data (``-Dpackage_data``)
"""""""""""""""""""""""""""""""""

* Default: ``auto``
* Options: ``auto``, ``true``, ``false``

Packages data into either a glob or a ``.zip`` depending on if ``-Denable_zip``
is ``true`` (see below).

.. code:: sh

   meson configure build/ -Dpackage_data=false

Enable ZIP Loading (``-Denable_zip``)
"""""""""""""""""""""""""""""""""""""

* Default: ``true```
* Options: ``true``, ``false``

Controls whether or not Taisei can load game data (textures, shaders, etc) from
``.zip`` files. Useful for distribution and packaging.

.. code:: sh

   meson configure build/ -Denable_zip=false

Strict Compiler Warnings (``-Dwerror``)
"""""""""""""""""""""""""""""""""""""""

* Default: ``false``
* Options: ``true``, ``false``

This option forces stricter checks against Taisei's codebase to ensure code
health, treating all ``Warnings`` as ``Errors`` in the code.

It's highly recommended to **enable** (i.e: ``true``) this whenever developing
for the engine. Sometimes, it's overly-pedantic, but much of the time, it
provides useful advice. (For example, it can detect potential null-pointer
exceptions that may not be obvious to the human eye.)

.. code:: sh

   meson configure build/ -Dwerror=true

Deprecation Warnings (``-Ddeprecation_warnings``)
"""""""""""""""""""""""""""""""""""""""""""""""""

* Default: ``(null)``
* Options: ``error``, ``no-error``, ``ignore``

Sets deprecation warnings to either hard-fail (``error``), print as warnings but
not trigger full errors if ``-Dwerror=true`` (``no-error``), and otherwise
ignore them (``ignore``).

Generally, ``no-error`` is the recommended default when using ``-Dwerror=true``.

.. code:: sh

   meson configure build/ -Ddeprecation_warnings=no-error

In-Game Developer Options (``-Ddeveloper``)
"""""""""""""""""""""""""""""""""""""""""""

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

Sets the type of build. ``debug`` enables several additional debugging features,
as well as reduced optimizations and more debugging symbols.

.. code:: sh

   meson configure build/ -Dbuildtype=debug

Assertions (``-Db_ndebug``)
"""""""""""""""""""""""""""

* Default: ``true``
* Options: ``true``, ``false``

The name of this flag is opposite of what you'd expect. Think of it as "Not
Debugging". It controls the ``NDEBUG`` declaration which is responsible for
deactivating ``assert()`` functions.

Setting to ``false`` will *enable* assertions (i.e: good for debugging).

Keep ``true`` during release.

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
version of Xcode. If it fails to launch for you, ensure that the version number
is correct by browsing to the parent directory of ``../clang``.

Then, you can launch Taisei's binary from the command line (using macOS as an
example):

.. code:: sh

   /path/to/Taisei.app/Contents/MacOS/Taisei

Link-Time Optimizations (``-Db_lto``)
"""""""""""""""""""""""""""""""""""""

* Default: ``true``
* Options: ``true``, ``false``

Link-time optimizations (LTO) increase build times, but also increase
performance. For quicker build times during development, you can disable it.
For release builds, this should be keep ``true``.

See: `Interprocedural Optimization <https://en.wikipedia.org/wiki/Interprocedural_optimization#WPO_and_LTO>`__

.. code:: sh

   meson configure build/ -Db_lto=false

Binary Striping (``-Dstrip``)
"""""""""""""""""""""""""""""

* Default: ``true``
* Options: ``true``, ``false``

This option prevents stripping of the `taisei` binary, leading to faster build
times and keeping debugging symbols in place. There is a theoretical performance
hit with this option enabled, but it can help with building during development.

Keep this ``true`` during releases.

.. code:: sh

   meson configure build/ -Db_strip=false

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

Sets the default renderer to use when Taisei launches.

.. code:: sh

   # for GL 3.3 (default)
   meson configure build/ -Dr_default=gl33
   # for GL ES 3.0
   meson configure build/ -Dr_default=gles30
   # for GL ES 2.0
   meson configure build/ -Dr_default=gles20

The renderer can be switched in the installed ``taisei`` binary.

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

Building ANGLE (Optional)
-------------------------

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

Using ``-Dinstall_angle`` and ``-Dangle_lib*`` (see above), ``meson`` will copy
those files over into the package itself when running the packaging steps.

Packaging
---------

Archive Types
"""""""""""""

Taisei supports a wide variety of packaging and archive types, including
``.zip``, ``.tar.gz``, ``.dmg`` (for macOS), ``.exe`` (for Windows), among
others.

Examples
""""""""

Linux
'''''

Compiling on Linux for Linux is fairly straightforward. We have ``meson``
machine configuration files provided for covering most of the basic settings
when building for Linux.

.. code:: sh

   meson setup build/ --native-file=misc/ci/linux-x86_64-build-release.ini
   meson compile -C build/

The ``--native-file`` contains many of the recommended default options for
building releases, including using ``--wrap-mode=forcefallback`` to force the
use of ``meson`` wrap dependenices (as mentioned earlier).

You can then package Taisei into ``.tar.xz`` (or any other ``.tar.*`` style
archive) by using the ``ninja`` command, which is typically installed alongside
``meson``.

.. code:: sh

   ninja txz -C build/

This will output a ``.tar.xz`` package inside ``build/``, which you can then
copy and distribute.

macOS
'''''

Taisei is released as a ``.dmg`` package for macOS. You can also build for both
x64 (Intel) and ARM64 (Apple Silicon, experimental).

Depending on what Mac you're compiling on or for, some options may change, so
pay special attention to the distinction between ``--native-file`` and
``--cross-file``.

Intel
^^^^^

Choose only one of the ``setup`` options depending on your hardware.

.. code:: sh

   # for building on Intel for Intel
   meson setup build/ --native-file=misc/ci/macos-x86_64-build-release.ini
   # for building on Apple Silicon for Intel
   meson setup build/ --cross-file=misc/ci/macos-x86_64-build-release.ini
   # compile
   meson compile -C build/


Apple Silicon
^^^^^^^^^^^^^

Again, choose only one of the ``setup`` options depending on your hardware.

.. code:: sh

   # for building on Apple Silicon for Apple Silicon
   meson setup build/ --native-file=misc/ci/macos-aarch64-build-release.ini
   # for building on Intel for Apple Silicon
   meson setup build/ --cross-file=misc/ci/macos-aarch64-build-release.ini
   # compile
   meson compile -C build/

Packaging
^^^^^^^^^

With ``create-dmg`` installed through ``brew``, you can create a pretty-looking
``.dmg`` file.

.. code:: sh

   ninja dmg -C build/

This will output a ``.dmg`` package inside ``build/``, which you can then
copy and distribute.

Windows
'''''''

As mentioned previously, it's recommended to use Linux when building for
Windows, utilizing the ``llvm-mingw`` toolchain.

* TODO

Emscripten
''''''''''

Emscripten relies on `emsdk <https://github.com/emscripten-core/emsdk>`__
to cross-compile for web browsers into `WASM <https://webassembly.org/>`__.

Follow the documentation there for more information, but here is a basic guide
to get you going.

.. code:: sh

   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk/
   ./emsdk.py install 2.0.27
   ./emsdk.py activate 2.0.27
   # follow the instructions it presents to you, but follow the one that looks similar to this
   source "/path/to/emsdk/emsdk_env.sh"

This will set up your environment to use ``emsdk`` as your toolchain. You'll
need to either set the ``source`` command in your shell's settings or re-run
it every time you open your terminal.

Since ``emscripten`` is its own separate platform, you are *always*
cross-compiling for it. (Hence, ``--cross-file``.)

.. code:: sh

   meson setup build/ --cross-file=misc/ci/emscripten-build.ini
   meson compile -C build/

You can then zip it up for uploading to a server.

.. code:: sh

   ninja zip -C build/

It will output your ``Taisei*.zip`` to ``build/``.


Switch (Homebrew)
'''''''''''''''''

Building for Switch requires the use of special Switch tools.

* TODO
