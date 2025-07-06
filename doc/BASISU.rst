Basis Universal
===============

.. contents::

Intro
-----

`Basis Universal <https://github.com/taisei-project/basis_universal>`__ is a GPU texture compression system with ability
to transcode images into a wide variety of formats at runtime from a single source. This document explains how to author
Basis Universal (``.basis``) textures for Taisei, and highlights limitations and caveats of our Basis support.

First-time setup
----------------

Step 1: The encoder
~~~~~~~~~~~~~~~~~~~

Compile the encoder and put it somewhere in your ``PATH``. Assuming you have a working and up to date clone of the
Taisei repository, including submodules, this can be done like so on Linux:

.. code:: sh

   cd subprojects/basis_universal
   meson setup --buildtype=release -Db_lto=true -Dcpp_args=-march=native build
   meson compile -C build basisu
   ln -s "$PWD/build/basisu" ~/.local/bin

Verify that the encoder is working by running ``basisu``. It should print a long list of options. If the command is not
found, make sure ``~/.local/bin`` is in your ``PATH``, or choose another directory that is.

The optimization options in ``meson setup`` are optional but highly recommended, as the encoding process can be quite
slow.

It’s also possible to use `the upstream encoder <https://github.com/BinomialLLC/basis_universal>`__, which may be
packaged by your distribution. However, this is not recommended. As of 2020-08-06, the upstream encoder is missing some
important performance optimizations; see
`BinomialLLC/basis_universal#105 <https://github.com/BinomialLLC/basis_universal/pull/105>`__
`BinomialLLC/basis_universal#112 <https://github.com/BinomialLLC/basis_universal/pull/112>`__
`BinomialLLC/basis_universal#113 <https://github.com/BinomialLLC/basis_universal/pull/113>`__.

Step 2: The wrapper
~~~~~~~~~~~~~~~~~~~

The ``mkbasis`` wrapper script is what you’ll actually use to create ``.basis`` files. Simply symlink it into your
``PATH``:

.. code:: sh

   ln -s "$PWD/scripts/mkbasis.py" ~/.local/bin/mkbasis

Verify that it works by running ``mkbasis``.

Encoding TL;DR
--------------

Encode a **diffuse or ambient map** (sRGB data, decoded to linear when sampled in a shader):

.. code:: sh

   # Outputs to foo.basis
   mkbasis foo.png

   # Outputs to /path/to/bar.basis
   mkbasis foo.png -o /path/to/bar.basis

Encode a **tangent-space normal map** (special case):

.. code:: sh

   mkbasis foo.png --normal

Encode a **roughness map** (single-channel linear data):

.. code:: sh

   mkbasis foo.png --channels=r --linear
   # Equivalent to:
   mkbasis foo.png --r --linear

Encode **RGBA** color data and **pre-multiply alpha**:

.. code:: sh

   mkbasis foo.png --channels=rgba
   # Equivalent to:
   mkbasis foo.png --rgba

Encode **Gray+Alpha** data and **pre-multiply alpha**:

.. code:: sh

   mkbasis foo.png --channels=gray-alpha
   # Equivalent to:
   mkbasis foo.png --gray-alpha

Do **not** pre-multiply alpha:

.. code:: sh

   mkbasis foo.png --no-multiply-alpha

Sacrifice quality to speed up the encoding process:

.. code:: sh

   mkbasis foo.png --fast

For a complete list of options and their default values, see

.. code:: sh

   mkbasis --help

Encoding details
----------------

Encoding modes
~~~~~~~~~~~~~~

Basis Universal supports two very different encoding modes: ETC1S and UASTC. The primary difference between the two is
the size/quality trade-off.

ETC1S is the default mode. It offers medium/low quality and excellent compression.

UASTC has significantly higher quality, but much larger file sizes. UASTC-encoded Basis files must also be additionally
compressed with an LZ-based scheme, such as deflate (zlib). Zopfli-compressed UASTC files are roughly 4 times as large
as their ETC1S equivalents (including mipmaps), comparable to the source file stored with lossless PNG or WebP
compression.

Although UASTC should theoretically work, it has not been well tested with Taisei yet. The ``mkbasis`` wrapper also does
not apply LZ compression to UASTC files automatically yet, and Taisei wouldn’t pick them up either (unless they are
stored compressed inside of a ``.zip`` package). If you want to use UASTC nonetheless, pass ``--uastc`` to ``mkbasis``.

*TODO*

Caveats and limitations
-----------------------

*TODO*
