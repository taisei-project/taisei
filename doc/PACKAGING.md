Taisei Project - Packaging
==========================

Some helpful tips on packaging Taisei for various platforms.

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

   meson setup build/ -Dbuild.cpp_std=gnu++14 --cross-file=misc/ci/emscripten-build.ini
   meson compile -C build/

**NOTE**: ``-Dbuild.cpp_std`` changes the C++ standard used to compile the
project. For Emscripten, it's necessary to set the project to ``gnu++14``
(as opposed to the default of ``gnu++11``) for certain submodules to compile
correctly with ``emcc`` from ``emsdk``. This is *not* typically required on
other platforms.

You can then zip it up for uploading to a server.

.. code:: sh

   ninja zip -C build/

It will output your ``Taisei*.zip`` to ``build/``.


Switch (Homebrew)
'''''''''''''''''

Building for Switch requires the use of special Switch tools.

* TODO
