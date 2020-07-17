Taisei
======

.. contents::

Introduction
------------

Taisei Project is an open source clone of the Tōhō Project series. Tōhō is a one-man
project of shoot-em-up games set in an isolated world full of Japanese folklore.

For gameplay instructions, read `this <doc/GAME.rst>`__.

For the story, read `this <doc/STORY.txt>`__. (Warning: spoilers!)

Installation
------------

You can find precompiled binaries of the complete game on the
`Releases <https://github.com/taisei-project/taisei/releases>`__ page on
GitHub, available for Windows (x86/x64), Linux, and macOS. An experimental
build for Nintendo Switch (homebrew) also exists (use at your own risk).

Source Code & Development
-------------------------

You can obtain the source code from the
`Releases <https://github.com/taisei-project/taisei/releases>`__ page on
GitHub, alongside the binaries.

**Important:** *do not* grab GitHub's auto-generated source archives, as those
do not contain the required submodules for compiling the project. This only
applies to versions v1.3 and above.

If you downloaded Taisei using `git clone`, make sure the submodules are
initialized:

::

    git submodule init
    git submodule update

This step needs to be done just once, and can be skipped if you specified the
``--recursive`` or ``--recurse-submodules`` option when cloning.

**Important:** You should also run ``git submodule update`` whenever you pull in
new code, checkout another branch, etc. The ``pull`` and ``checkout`` helper
scripts can do that for you automatically.

For more in-depth development and compiling instructions, see:
`Development Guide <doc/DEVELOPMENT.rst>`__.

Dependencies
^^^^^^^^^^^^

-  OpenGL >= 3.3 or OpenGL ES >= 3.0 or OpenGL ES >= 2.0 (with some extensions)
-  SDL2 >= 2.0.6
-  SDL2_mixer >= 2.0.4
-  freetype2
-  libpng >= 1.5.0
-  libwebpdecoder >= 0.5 or libwebp >= 0.5
-  libzip >= 1.2
-  opusfile
-  zlib

Optional:

-  OpenSSL (for a better SHA-256 implementation; used in shader cache)
-  SPIRV-Cross >= 2019-03-22 (for OpenGL ES backends)
-  libshaderc (for OpenGL ES backends)


Replays, Screenshots, and Settings Locations
--------------------------------------------

Taisei stores all data in a platform-specific directory:

-  On **Windows**, this will probably be ``%APPDATA%\taisei``
-  On **macOS**, it's ``$HOME/Library/Application Support/taisei``
-  On **Linux**, **\*BSD**, and most other **Unix**-like systems, it's
   ``$XDG_DATA_HOME/taisei`` or ``$HOME/.local/share/taisei``

This is referred to as the **Storage Directory**. You can set the environment
variable ``TAISEI_STORAGE_PATH`` to override this behaviour.


Troubleshooting
---------------

If you're having issues with low framerates, sound playback issues, or gamepad
support, please see the `Troubleshooting Guide <doc/TROUBLESHOOTING.rst>`__.


Contact
-------

-  https://taisei-project.org/

-  `#taisei-project on Freenode <irc://irc.freenode.org/taisei-project>`__

-  `Our server on Discord <https://discord.gg/JEHCMzW>`__
