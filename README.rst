Taisei
======

.. contents::

Introduction
------------

Taisei is an open clone of the Tōhō Project series. Tōhō is a one-man project of
shoot-em-up games set in an isolated world full of Japanese folklore.

Installation
------------

Dependencies
^^^^^^^^^^^^

-  OpenGL >= 3.3 or OpenGL ES >= 3.0 or OpenGL ES >= 2.0 (with some extensions)
-  `cglm <https://github.com/recp/cglm>`__ >= 0.7.8
-  SDL2 >= 2.0.10
-  freetype2
-  libpng >= 1.5.0
-  libwebpdecoder >= 0.5 or libwebp >= 0.5
-  libzip >= 1.5.0 (>= 1.7.0 recommended)
-  libzstd >= 1.3.0
-  opusfile
-  zlib

Optional:

-  OpenSSL (for a better SHA-256 implementation; used in shader cache)
-  SPIRV-Cross >= 2019-03-22 (for OpenGL ES backends)
-  libshaderc (for OpenGL ES backends)
-  GameMode headers (Linux only; for automatic `GameMode
   <https://github.com/FeralInteractive/gamemode>`__ integration)

Build-only dependencies
^^^^^^^^^^^^^^^^^^^^^^^

-  Python >= 3.7
-  meson >= 0.53.0
-  [python-zstandard](https://github.com/indygreg/python-zstandard)

Optional:

-  docutils (for documentation)

Obtaining the source code
^^^^^^^^^^^^^^^^^^^^^^^^^

Stable releases
"""""""""""""""

You can find the source tarballs at the
`Releases <https://github.com/taisei-project/taisei/releases>`__ section on
Github. **Do not** grab Github's auto-generated source archives, those do not
contain the required submodules. This only applies for versions v1.3 and above.

Latest code from git
""""""""""""""""""""

If you cloned Taisei from git, make sure the submodules are initialized:

::

    git submodule init
    git submodule update

This step needs to be done just once, and can be skipped if you specified the
``--recursive`` or ``--recurse-submodules`` option when cloning.

**Important:** You should also run ``git submodule update`` whenever you pull in
new code, checkout another branch, etc. The ``pull`` and ``checkout`` helper
scripts can do that for you automatically.

Compiling from source
^^^^^^^^^^^^^^^^^^^^^

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

The OpenGL ES 3.0 backend is not built by default. To enable it, do:

::

    meson configure -Dr_gles30=true -Dshader_transpiler=true

See `here <doc/ENVIRON.rst>`__ for information on how to activate it.
Alternatively, do this to make GLES 3.0 the default backend:

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


Where are my replays, screenshots and settings?
-----------------------------------------------

Taisei stores all data in a platform-specific directory:

-  On **Windows**, this will probably be ``%APPDATA%\taisei``
-  On **macOS**, it's ``$HOME/Library/Application Support/taisei``
-  On **Linux**, **\*BSD**, and most other **Unix**-like systems, it's
   ``$XDG_DATA_HOME/taisei`` or ``$HOME/.local/share/taisei``

This is referred to as the **Storage Directory**. You can set the environment
variable ``TAISEI_STORAGE_PATH`` to override this behaviour.

Game controller support
-----------------------

Taisei uses SDL2's unified GameController API. This allows us to correctly
support any device that SDL recognizes by default, while treating all of them
the same way. This also means that if your device is not supported by SDL, you
will not be able to use it unless you provide a custom mapping. If your
controller is listed in the settings menu, then you're fine. If not, read on.

An example mapping string looks like this:

::

    03000000ba2200002010000001010000,Jess Technology USB Game Controller,a:b2,b:b1,back:b8,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:,leftshoulder:b4,lefttrigger:b6,leftx:a0,lefty:a1,rightshoulder:b5,righttrigger:b7,rightx:a3,righty:a2,start:b9,x:b3,y:b0,

There are a few ways to generate a custom mapping:

-  You can use the
   `controllermap <https://aur.archlinux.org/packages/controllermap>`__ utility,
   which `comes with SDL source code
   <https://hg.libsdl.org/SDL/file/68a767ae3a88/test/controllermap.c>`__.
-  If you use Steam, you can configure your controller there. Then you can add
   Taisei as a non-Steam game; run it from Steam and everything should *just
   work™*. In case you don't want to do that, find ``config/config.vdf`` in your
   Steam installation directory, and look for the ``SDL_GamepadBind`` variable.
   It contains a list of SDL mappings separated by line breaks.
-  You can also try the `SDL2 Gamepad Tool by General Arcade
   <http://www.generalarcade.com/gamepadtool/>`__. This program is free to use,
   but not open source.
-  Finally, you can try to write a mapping by hand. You will probably have to
   refer to the SDL documentation. See `gamecontrollerdb.txt
   <misc/gamecontrollerdb/gamecontrollerdb.txt>`__ for some more examples.

Once you have your mapping, there are two ways to make Taisei use it:

-  Create a file named ``gamecontrollerdb.txt`` where your config, replays and
   screenshots are, and put each mapping on a new line.
-  Put your mappings in the environment variable ``SDL_GAMECONTROLLERCONFIG``,
   also separated by line breaks. Other games that use the GameController API
   will also pick them up.

When you're done, please consider contributing your mappings to
`SDL <https://libsdl.org/>`__,
`SDL_GameControllerDB <https://github.com/gabomdq/SDL_GameControllerDB>`__,
and `us <https://github.com/taisei-project/SDL_GameControllerDB>`__, so
that other people can benefit from your work.

Also note that we currently only handle input from analog axes and digital
buttons. Hats, analog buttons, and anything more exotic will not work, unless
remapped.

Troubleshooting
---------------

Sound problems (Linux)
^^^^^^^^^^^^^^^^^^^^^^

If your sound becomes glitchy, and you encounter lot of console messages like:

::

    ALSA lib pcm.c:7234:(snd_pcm_recover) underrun occurred

it seems like you possibly have broken ALSA configuration. This may be fixed by
playing with parameter values of ``pcm.dmixer.slave`` option group in
``/etc/asound.conf`` or wherever you have your ALSA configuration.
Commenting ``period_time``, ``period_size``, ``buffer_size``, ``rate`` may give
you the first approach to what to do.

Contact
-------

-  https://taisei-project.org/

-  `#taisei-project on Freenode <irc://irc.freenode.org/taisei-project>`__

-  `Our server on Discord <https://discord.gg/JEHCMzW>`__
