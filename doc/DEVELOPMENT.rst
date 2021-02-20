Taisei Project - Compiling FAQ
==============================

.. contents::

Introduction
------------

This document contains some important tips on compiling and developing for
specific platforms.

Compiling The Source
--------------------

Basic Build Instructions
^^^^^^^^^^^^^^^^^^^^^^^^

-  C (C11) compiler (``gcc``, ``clang``, etc)
-  Python >= 3.5
-  meson >= 0.53.0
-  ninja >= 1.10.0
-  docutils

You can optionally install `other dependencies <../README.rst#dependencies>`__,
but the build system will pull in everything else you might need otherwise.

First, you'll want to set up a prefix for where the game's binaries  will be
installed. For starting out development, you're better off installing it to
your HOME directory.

::

    export TAISEI_PREFIX=/home/$user/bin

To build and install Taisei on \*nix:

::

    cd taisei/
    make all # sets up, compiles, and installs Taisei


Development
-----------

Build Options
^^^^^^^^^^^^^

Game Data Path
""""""""""""""

When compiling with ``TAISEI_PREFIX`` set, game file data will be
installed to ``$TAISEI_PREFIX/share/taisei/``, and this path will be built
*statically* into the executable. You can decide whether or not you want this
based on your own preferences. Alternatively, you can install game data
relatively as well:

::

    make setup/install-relative

Which will cause save game data to be installed to:

::

    $TAISEI_PREFIX/taisei/
    $TAISEI_PREFIX/data/

Note that ``install relative`` is always set when building for Windows.

Debugging
"""""""""

You can enable debugging options/output for development purposes:

::

    make setup/debug


Faster Builds
"""""""""""""

This option also helps for speeding up build times, although there is a
theoretical reduction in performance with these options:

::

    make setup/build-speed


Developer Mode
""""""""""""""

For debugging actual gameplay, you can set this option and it will enable cheats
and other 'fast-forward' options by the pressing keys defined in
``src/config.h``.

::

    make setup/developer

Stack Trace Debugging
"""""""""""""""""""""

This is useful for debugging crashes in the game. It uses
`AddressSanitizer <https://github.com/google/sanitizers/wiki/AddressSanitizer>__`:

::

    make setup/debug-asan

Depending on your platform, you may need to specify the specific library binary
to use to launch ASan appropriately. Using macOS as an example:

::

    export DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/12.0.0/lib/darwin/libclang_rt.asan_osx_dynamic.dylib

The ``../12.0.0/..`` in the path of ``DYLD_INSERT_LIBRARIES`` changes with each
version of XCode. If it fails to launch for you, ensure that the version number
is correct by browsing to the parent directory of ``../clang``.

Then, you can launch Taisei's binary from the command line (using macOS as an example):

::

    /path/to/Taisei.app/Contents/MacOS/Taisei


OpenGL ES Support (Optional)
""""""""""""""""""""""""""""

3.0
'''


The OpenGL ES 3.0 backend is not built by default. To enable it, do:

::

    make setup/gles/30

2.0
'''

An experimental OpenGL ES 2.0 backend can be enabled similarly, using:

::

    make setup/gles/20

However, GLES 2.0 requires a few extensions to be present on your system
to function correctly, most notably:

- ``OES_depth_texture`` or ``GL_ANGLE_depth_texture``
- ``OES_standard_derivatives``
- ``OES_vertex_array_object``
- ``EXT_frag_depth``
- ``EXT_instanced_arrays`` or ``ANGLE_instanced_arrays`` or
  ``NV_instanced_arrays``

ANGLE (Windows/macOS)
'''''''''''''''''''''

For Windows and macOS, you will need Google's ANGLE library for both ES 3.0 and
2.0. You'll need to check out
`ANGLE <https://github.com/google/angle>`__ and build it first. Refer to their
documentation on how to do that.

Once you've compiled ANGLE, enable it with:

::

    export LIBGLES=/path/to/libGLESv2.{dll,dylib}
    export LIBEGL=/path/to/libEGL.{dll,dylib}
    make setup/gles/angle
    make compile
    make install

Ensure you use the correct file extension for your platform. (``.dll`` for
Windows, ``.dylib`` for macOS.)

This will copy the file over into the package itself.

Coding Style
^^^^^^^^^^^^

In the ``*.c`` files, tabs are used. In the ``meson.build`` and ``*.py`` files,
spaces are used.

To help you abide by this standard, you should install
`EditorConfig <https://github.com/editorconfig>`__ for your preferred editor of
choice, and load in the file found at ``.editorconfig`` in the root of the
project.

Platform-Specific Tips
^^^^^^^^^^^^^^^^^^^^^^

Linux (Debian/Ubuntu)
"""""""""""""""""""""

On an apt-based system (Debian/Ubuntu), ensure you have build dependencies
installed:

::

    apt-get install meson cmake build-essential
    apt-get install libsdl2-dev libsdl2-mixer-dev libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev libwayland-dev

macOS
"""""

On macOS, you need to begin with installing the Xcode Command Line Tools:

::

    xcode-select --install

There are additional command line tools that you'll need. You can acquire those
by using `Homebrew <https://brew.sh/>`__.

Follow the instructions for installing Homebrew, and then install the following
tools:

::

    brew install meson cmake pkg-config docutils imagemagick pygments

The following dependencies are technically optional, and can be pulled in at
build-time, but you're better off installing them yourself to reduce compile
times:

::

    brew install freetype2 libzip opusfile libvorbis webp sdl2

As of 2020-02-18, you should **not** install the following packages via
Homebrew, as the versions available do not compile against Taisei correctly.
If you're having mysterious errors, ensure that they're not installed.

* ``spirv-tools``
* ``spirv-cross``
* ``sdl2_mixer``

Remove them with:

::

    brew remove spirv-tools spirv-cross sdl2_mixer


Taisei-compatible versions are bundled and will be pulled in at compile time.

In addition, if you're trying to compile on an older version of macOS
(e.x: <10.12), SDL2 may not compile correctly on Homebrew (as of 2019-02-19).
Let ``meson`` pull in the corrected version for you via subprojects.

**NOTE:** While Homebrew's optional dependencies greatly improve compile times,
if you can't remove packages that give you errors from your system for whatever
reason, you can force ``meson`` to use its built-in subprojects by using the
following option:

::

    make setup/fallback

Windows
"""""""

While the game itself officially supports Windows, building the project
directly on Windows is a bit difficult to set up due to the radically different
tooling required.

However, you can still compile on a Windows-based computer by leveraging Windows
10's
`Windows For Linux (WSL) Subsystem <https://docs.microsoft.com/en-us/windows/wsl/install-win10>__`
to cross-compile to Windows.

Graphical Assets
----------------

Taisei's GFX library is made up of a collection of sprites, shaders, and a few
3D models. The 3D models are almost exclusively used for background scenery
(and a few other places, like the HUD), while the sprites are used in everything
from UI elements, character portraits, to the
danmaku bullets themselves.

To modify the 3D models, you'll need `Blender <https://blender.org>`__, which is
free and open source. Look for tutorials on YouTube for how to use it. The
models themselves are located in ``resources/00-taisei.pkgdir/models``.

Taisei uses ``.obj`` for its 3D models. To export ``.obj`` files from Blender,
use ``File -> Export -> Wavefront (.obj)``. Ensure that the following settings
are used:

::

    Include
        Objects as OBJ Objects: ENABLED

    Transform
        Forward: Y Forward
        Up: -Z Up

    Geometry
        Write Materials: DISABLED
        Triangulate Faces: ENABLED

Music and sound effects are located in ``resources/00-taisei.pkgdir/sfx``.

For sprites, any image editor will do. Sprites are located in ``atlas``.
However, to have sprites properly appear in Taisei, you'll need a few packages
and tools first to rebuild the atlas so the game can load them properly.

You'll need ``rectpack`` and ``pillow`` from ``Python PIP``:

::

    pip3 install rectpack pillow

You'll also need to download (and/or compile) and install
`Leanify <https://github.com/JayXon/Leanify>`__.

You'll need to run one of the following commands to regenerate the ``atlas``
once the sprites have been modified. Pay attention to which directory you've
made your changes in (such as ``common_ui``) and use the appropriate command.`

::

    ninja gen-atlas-common_ui
    ninja gen-atlas-common
    ninja gen-atlas-portraits

Or, to regenerate *everything*:

::

    ninja gen-atlases

That will regenerate the files needed for your new sprites to appear correctly.

*Generally speaking*, Taisei prefers ``.webp`` as the final product, but can
convert ``.png`` into ``.webp`` using the above ``ninja gen-atlas*`` commands.

Compiling Issues
----------------

* `-Wunused-variable` - if you get an error compiling your code, but you're 100%
sure that you've actually used the variable, chances are you're using that
variable in an `assert()` and are compiling with `clang`.

`clang` won't recognize that the variable is actually being used in an `assert()`.

You can use the macro `attr_unused` to bypass that warning. This:

::

    int x = 0;
    assert(x == 0);

Becomes this:

::

    attr_unused int x = 0;
    assert(x == 0);
