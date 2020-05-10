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
-  meson >= 0.48.0 (build system; >=0.49.0 recommended)
-  ninja >= 1.7
-  docutils

You can optionally install `other dependencies <../README.rst#dependencies>`__,
but the build system will pull in everything else you might need otherwise.

To build and install Taisei on \*nix, just follow these steps:

::

    cd /path/to/taisei/source
    mkdir build
    cd build
    meson --prefix=$yourprefix ..
    ninja
    ninja install

This will install game data to ``$prefix/share/taisei/`` and build this
path *statically* into the executable. This might be a package
maintainer’s choice. Alternatively you may want to add
``-Dinstall_relative=true`` to get a relative structure like

::

    $prefix/taisei
    $prefix/data/

``install_relative`` is always set when building for Windows.


OpenGL ES Support (Optional)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The OpenGL ES 3.0 backend is not built by default. To enable it, do:

::

    meson configure -Dr_gles30=true -Dshader_transpiler=true

See `here <doc/ENVIRON.rst>`__ on how to manually activate it.

To set OpenGL ES 3.0 as the *default* renderer, do:

::

    meson configure -Dr_default=gles30

The OpenGL ES 2.0 backend can be enabled similarly, using ``gles20`` instead of
``gles30``. However, it requires a few extensions to function correctly, most
notably:

- ``OES_depth_texture`` or ``GL_ANGLE_depth_texture``
- ``OES_standard_derivatives``
- ``OES_vertex_array_object``
- ``EXT_frag_depth``
- ``EXT_instanced_arrays`` or ``ANGLE_instanced_arrays`` or
  ``NV_instanced_arrays``

For Windows and macOS, you will need Google's ANGLE library for both ES 3.0 and
2.0. You'll need to check out
`ANGLE <https://github.com/google/angle>`__ and build it first. Refer to their
documentation on how to do that.

Once you've compiled ANGLE, enable it with:

::

    meson -Dinstall_angle=true -Dangle_libegl=/path/to/libEGL.{dll,dylib}
    -Dangle_libgles=/path/to/libGLESv2.{dll,dylib}

Ensure you use the correct file extension for your platform. (``.dll`` for
Windows, ``.dylib`` for macOS.)

It'll install automatically with ``ninja install`` (as mentioned above).

Development
-----------

Build Options
^^^^^^^^^^^^^

You can enable debugging options/output for development purposes:

::

    meson configure -Dbuildtype=debug -Db_ndebug=false


This option also helps for speeding up build times:

::

    meson configure -Db_lto=false


For debugging actual gameplay, you can set this option and it will enable cheats
and other 'fast-forward' options by the pressing keys defined in
``src/config.h``.

::

    meson configure -Ddeveloper=true


Coding Style
^^^^^^^^^^^^

In the ``*.c`` files, tabs are used. In the ``meson.build`` and ``*.py`` files, spaces
are used.

To help you abide by this standard, you should install
`EditorConfig <https://github.com/editorconfig>`__ for your preferred editor of
choice, and load in the file found at ``.editorconfig`` in the root of the
project.


Platform-Specific Tips
^^^^^^^^^^^^^^^^^^^^^^

Windows
"""""""

TODO: complete this section.

Linux (Debian/Ubuntu)
"""""""""""""""""""""

On an apt-based system (Debian/Ubuntu), ensure you have build dependencies
installed:

::

    apt-get install meson cmake build-essential libsdl2-dev libsdl2-mixer-dev libogg-dev libopusfile-dev libpng-dev libzip-dev libx11-dev libwayland-dev


macOS
"""""

Manual
~~~~~

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


Taisei-compatible versions will be pulled in at compile time.

In addition, if you're trying to compile on an older version of macOS
(e.x: <10.12), SDL2 may not compile correctly on Homebrew (as of 2019-02-19).
Let ``meson`` pull in the corrected version for you via subprojects.

**NOTE:** While Homebrew's optional dependencies greatly improve compile times,
if you can't remove packages that give you errors from your system for whatever
reason, you can force ``meson`` to use its built-in subprojects by using the
following:

::

    meson --wrap-mode forcefallback


Automatic
~~~~~~~~~

To do the above steps automatically, run ``scripts/macos-deps-install.sh``.

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
