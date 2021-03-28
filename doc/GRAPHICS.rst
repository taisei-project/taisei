Graphical Assets
================

.. contents::

Introduction
------------

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

.. code:: sh

    pip3 install rectpack pillow

You'll also need to download (and/or compile) and install
`Leanify <https://github.com/JayXon/Leanify>`__.

You'll need to run one of the following commands to regenerate the ``atlas``
once the sprites have been modified. Pay attention to which directory you've
made your changes in (such as ``common_ui``) and use the appropriate command.`

.. code:: sh

    ninja gen-atlas-common_ui
    ninja gen-atlas-common
    ninja gen-atlas-portraits

Or, to regenerate *everything*:

.. code:: sh

    ninja gen-atlases

That will regenerate the files needed for your new sprites to appear correctly.

*Generally speaking*, Taisei prefers ``.webp`` as the final product, but can
convert ``.png`` into ``.webp`` using the above ``ninja gen-atlas*`` commands.

Compression
-----------

`Basis Universal <https://github.com/taisei-project/basis_universal>`__ is a GPU
texture compression system with ability to transcode images into a wide variety
of formats at runtime from a single source. This document explains how to author
Basis Universal (``.basis``) textures for Taisei, and highlights limitations and
caveats of our Basis support.

First-time setup
~~~~~~~~~~~~~~~~

Encoder
"""""""

Compile the encoder and put it somewhere in your ``PATH``. Assuming you have a
working and up to date clone of the Taisei repository, including submodules,
this can be done like so on Linux:

.. code:: sh

    cd subprojects/basis_universal
    meson setup --buildtype=release -Db_lto=true -Dcpp_args=-march=native build
    meson compile -C build basisu
    ln -s $PWD/build/basisu ~/.local/bin

Verify that the encoder is working by running ``basisu``. It should print a long
list of options. If the command is not found, make sure ``~/.local/bin`` is in
your ``PATH``, or choose another directory that is.

The optimization options in ``meson setup`` are optional but highly recommended,
as the encoding process can be quite slow.

It's also possible to use
`the upstream encoder <https://github.com/BinomialLLC/basis_universal>`__,
which may be packaged by your distribution. However, this is not recommended.
As of 2020-08-06, the upstream encoder is missing some important performance
optimizations; see
`BinomialLLC/basis_universal#105 <https://github.com/BinomialLLC/basis_universal/pull/105>`__
`BinomialLLC/basis_universal#112 <https://github.com/BinomialLLC/basis_universal/pull/112>`__
`BinomialLLC/basis_universal#113 <https://github.com/BinomialLLC/basis_universal/pull/113>`__.

Wrapper
"""""""

The ``mkbasis`` wrapper script is what you'll actually use to create ``.basis``
files. Simply symlink it into your ``PATH``:

.. code:: sh

    ln -s $PWD/scripts/mkbasis.py ~/.local/bin/mkbasis

Verify that it works by running ``mkbasis``.

Encoding
~~~~~~~~

Commands
""""""""

Encode a **diffuse or ambient map** (sRGB data, decoded to linear when sampled
in a shader):

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

Examples
""""""""

As an example:

.. code:: sh

   mkbasis --uastc reimu.png -o reimu.basis.zst && cp reimu.basis.zst /path/to/src/taisei/resources/00-taisei.pkgdir/gfx/

Details
~~~~~~~

Encoding Modes
""""""""""""""

Basis Universal supports two very different encoding modes: ETC1S and UASTC. The
primary difference between the two is the size/quality trade-off.

ETC1S is the default mode. It offers medium/low quality and excellent
compression.

UASTC has significantly higher quality, but much larger file sizes.
UASTC-encoded Basis files must also be additionally compressed with an LZ-based
scheme, such as deflate (zlib). Zopfli-compressed UASTC files are roughly 4
times as large as their ETC1S equivalents (including mipmaps), comparable to the
source file stored with lossless PNG or WebP compression.

Although UASTC should theoretically work, it has not been well tested with
Taisei yet. The ``mkbasis`` wrapper also does not apply LZ compression to UASTC
files automatically yet, and Taisei wouldn't pick them up either (unless they
are stored compressed inside of a ``.zip`` package). If you want to use UASTC
nonetheless, pass ``--uastc`` to ``mkbasis``.

*TODO*

Caveats & Limitations
"""""""""""""""""""""""

*TODO*

Animation Sequence Format
=========================

An animation is made up of a number of sprites and an ``.ani`` file which
contains all the metadata.

The ``.ani`` file needs to specify the number of sprites using the
``@sprite_count`` attribute. Then different animation sequences can be defined.

Animation sequences are chains of sprites that can be replayed in-game. For
example Cirno can either fly normally or flex while flying. In order for the
game to understand which sprites need to be shown in what order and time delay
you need to define a sequence for every action.

To define the action *right* of the player flying to the left for example,
you write into the file:

.. code:: c

    right = d5 0 1 2 3

Every key in the ``.ani`` file not starting with ``@`` corresponds to a
sequence.  The sequence specification itself is a list of frame indices. In the
example above, the right sequence will cycle frames 0-3. Everything that is not
a number like ``d5`` in the example is a parameter:

* ``d<n>`` sets the frame delay to n. This means every sprite index given is
  shown for n ingame frames.
* ``m`` toggles the mirroring of the following frames.
* ``m0,m1`` set the absolute mirroring of the following frames.

All parameters are persistent within one sequence spec until you change them.

More examples:

.. code:: c

    flap = d5 0 1 2 3 d2 4 5 6 7
    left = m d5 0 1 2 3
    alternateleftright = m d5 0 1 2 3 m 0 1 2 3

There are many possibilities to use ``d<n>`` to make animations look dynamic (or
strange)

Naming Conventions
------------------

The resource code does not require you to choose any specific names for your
sequences, but different parts of the game of course have to at some point.
The most common convention is calling the standard sequence "main". This is the
least you need to do for anything using an aniplayer, because the aniplayer
needs to know a valid starting animation.

If you have a sequence of the sprite going left or right, call it "left" and
"right". Player and fairy animations do this.

Look at existing files for more examples. Wriggle might be interesting for
complicated delay and queue trickery.

The documentation of the ``AniPlayer`` struct might also be interesting to
learn more about the internals.

